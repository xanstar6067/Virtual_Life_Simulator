#pragma once

// Memory pool for CB3 bots, grouped by field column.
//
// The field is processed column by column, and a bot's perception reads its neighbours. With ordinary heap
// allocation, births and deaths scatter bots over memory, and the same work can take several times longer
// waiting for cache misses (see CB3_PERFORMANCE.md). Here every region of ColumnsPerRegion columns has its own
// 64 KiB chunks, and a bot takes a slot in the region of the column where it is born. Bots processed together
// therefore share a few chunks that fit in L2. Objects never move, so no pointers have to be updated; a bot
// that travels far keeps its slot, and locality is restored as the population turns over.
//
// Allocation and release are thread-safe: a bot may die in another worker's strip than the one where its slot
// region lies. Each region has its own spin lock, held only for a few list operations. A chunk only reserves
// address space; its pages are committed as slots are first used, so a sparse population does not pay for
// whole chunks. A chunk is returned to the system as soon as it becomes empty, so clearing the world releases
// the memory. Committed pages of a partly used chunk stay committed and are reused by later births.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <Windows.h>
#include <immintrin.h>

#include "Settings.h"

namespace cb3::botpool
{
	// VirtualAlloc returns blocks aligned to 64 KiB, so a slot finds its chunk header by masking its address.
	constexpr size_t ChunkBytes = 64 * 1024;
	constexpr size_t PageBytes = 4 * 1024;
	constexpr int ColumnsPerRegion = 2;
	constexpr int NumColumnRegions = ((FieldCellsWidth) + ColumnsPerRegion - 1) / ColumnsPerRegion;
	// The last region is for bots created without a known column (spawned at random, loaded from a bot file).
	constexpr int NumRegions = NumColumnRegions + 1;
	constexpr int NoColumn = -1;

	struct Region;

	struct alignas(64) Chunk final
	{
		Region* region = nullptr;
		// Links of the region's list of chunks that still have free slots.
		Chunk* prev = nullptr;
		Chunk* next = nullptr;
		void* freeList = nullptr;
		uint32_t used = 0, capacity = 0, bumped = 0, slotSize = 0;
		// Bytes from the chunk start that are committed.
		uint32_t committed = 0;
	};

	struct alignas(64) Region final
	{
		std::atomic<bool> busy{ false };
		Chunk* partial = nullptr;

		void Lock() noexcept
		{
			while (busy.exchange(true, std::memory_order_acquire))
				while (busy.load(std::memory_order_relaxed))
					_mm_pause();
		}
		void Unlock() noexcept { busy.store(false, std::memory_order_release); }

		void Unlink(Chunk* c) noexcept
		{
			if (c->prev) c->prev->next = c->next;
			else partial = c->next;
			if (c->next) c->next->prev = c->prev;
			c->prev = c->next = nullptr;
		}
		void PushFront(Chunk* c) noexcept
		{
			c->prev = nullptr;
			c->next = partial;
			if (partial) partial->prev = c;
			partial = c;
		}
	};

	inline Region regions[NumRegions];
	inline std::atomic<size_t> chunkCount{ 0 };
	inline std::atomic<size_t> committedBytes{ 0 };

	// Commits pages up to `end` bytes from the chunk start. Called with the region lock held.
	inline bool CommitUpTo(Chunk* c, size_t end) noexcept
	{
		if (end <= c->committed) return true;
		end = (end + PageBytes - 1) & ~(PageBytes - 1);
		if (!VirtualAlloc(reinterpret_cast<char*>(c) + c->committed, end - c->committed, MEM_COMMIT, PAGE_READWRITE))
			return false;
		committedBytes.fetch_add(end - c->committed, std::memory_order_relaxed);
		c->committed = static_cast<uint32_t>(end);
		return true;
	}

	// Column used by a plain `new Bot(...)`; see ColumnHint.
	inline thread_local int columnHint = NoColumn;

	inline Region& RegionFor(int column) noexcept
	{
		return regions[(column >= 0 && column < (FieldCellsWidth)) ? column / ColumnsPerRegion : NumRegions - 1];
	}

	inline void* Allocate(size_t size, int column)
	{
		// Keep the alignment guarantee of the global operator new.
		size = (size + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
		if (size > ChunkBytes - sizeof(Chunk)) throw std::bad_alloc();
		Region& r = RegionFor(column);
		r.Lock();
		Chunk* c = r.partial;
		if (!c)
		{
			void* memory = VirtualAlloc(nullptr, ChunkBytes, MEM_RESERVE, PAGE_READWRITE);
			if (!memory || !VirtualAlloc(memory, PageBytes, MEM_COMMIT, PAGE_READWRITE))
			{
				if (memory) VirtualFree(memory, 0, MEM_RELEASE);
				r.Unlock();
				throw std::bad_alloc();
			}
			committedBytes.fetch_add(PageBytes, std::memory_order_relaxed);
			c = new (memory) Chunk;
			c->committed = static_cast<uint32_t>(PageBytes);
			c->region = &r;
			c->slotSize = static_cast<uint32_t>(size);
			c->capacity = static_cast<uint32_t>((ChunkBytes - sizeof(Chunk)) / size);
			r.PushFront(c);
			chunkCount.fetch_add(1, std::memory_order_relaxed);
		}
		// All pooled objects are bots; a different size would mean the pool is used for another class.
		if (c->slotSize != size) { r.Unlock(); std::abort(); }
		void* p;
		if (c->freeList)
		{
			p = c->freeList;
			c->freeList = *static_cast<void**>(p);
		}
		else
		{
			const size_t offset = sizeof(Chunk) + size_t(c->bumped) * c->slotSize;
			if (!CommitUpTo(c, offset + c->slotSize)) { r.Unlock(); throw std::bad_alloc(); }
			p = reinterpret_cast<char*>(c) + offset;
			++c->bumped;
		}
		if (++c->used == c->capacity) r.Unlink(c);
		r.Unlock();
		return p;
	}

	inline void Free(void* p) noexcept
	{
		if (!p) return;
		Chunk* c = reinterpret_cast<Chunk*>(reinterpret_cast<uintptr_t>(p) & ~(uintptr_t(ChunkBytes) - 1));
		Region& r = *c->region;
		r.Lock();
		const bool wasFull = c->used == c->capacity;
		*static_cast<void**>(p) = c->freeList;
		c->freeList = p;
		if (--c->used == 0)
		{
			if (!wasFull) r.Unlink(c);
			r.Unlock();
			committedBytes.fetch_sub(c->committed, std::memory_order_relaxed);
			VirtualFree(c, 0, MEM_RELEASE);
			chunkCount.fetch_sub(1, std::memory_order_relaxed);
			return;
		}
		if (wasFull) r.PushFront(c);
		r.Unlock();
	}

	// Sets the column for plain `new Bot(...)` calls in this scope on this thread, e.g. when an object is
	// created before its coordinates are read from a file.
	struct ColumnHint final
	{
		explicit ColumnHint(int column) noexcept : saved(columnHint) { columnHint = column; }
		~ColumnHint() { columnHint = saved; }
		ColumnHint(const ColumnHint&) = delete;
		ColumnHint& operator=(const ColumnHint&) = delete;
	private:
		int saved;
	};

	// Argument of the placement form: `new (botpool::AtColumn{ x }) Bot(x, y, ...)`.
	struct AtColumn final { int column; };

	// Committed memory of all chunks (what the pool costs in RAM).
	inline size_t AllocatedBytes() noexcept { return committedBytes.load(std::memory_order_relaxed); }
}
