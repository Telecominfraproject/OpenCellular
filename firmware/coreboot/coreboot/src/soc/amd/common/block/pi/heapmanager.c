/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <amdblocks/agesawrapper.h>
#include <amdlib.h>
#include <arch/acpi.h>
#include <amdblocks/BiosCallOuts.h>
#include <cbmem.h>
#include <string.h>

static void *agesa_heap_base(void)
{
	return cbmem_add(CBMEM_ID_RESUME_SCRATCH, BIOS_HEAP_SIZE);
}

static void EmptyHeap(int unused)
{
	void *BiosManagerPtr = agesa_heap_base();
	memset(BiosManagerPtr, 0, BIOS_HEAP_SIZE);
}

/*
 * Name			FindAllocatedNode
 * Brief description	Find an allocated node that matches the handle.
 * Input parameter	The desired handle.
 * Output parameters
 *	pointer		Here is returned either the found node or the last
 *			allocated node if the handle is not found. This is
 *			intentional, as the field NextNode of this node will
 *			have to be filled with the offset of the node being
 *			created in procedure agesa_AllocateBuffer().
 *	Status		Indicates if the node was or was not found.
 */
static AGESA_STATUS FindAllocatedNode(uint32_t handle,
				BIOS_BUFFER_NODE **last_allocd_or_match)
{
	UINT32              AllocNodeOffset;
	UINT8               *BiosHeapBaseAddr;
	BIOS_BUFFER_NODE    *AllocNodePtr;
	BIOS_HEAP_MANAGER   *BiosHeapBasePtr;
	AGESA_STATUS        Status = AGESA_SUCCESS;

	BiosHeapBaseAddr = agesa_heap_base();
	BiosHeapBasePtr = (BIOS_HEAP_MANAGER *)BiosHeapBaseAddr;

	AllocNodeOffset = BiosHeapBasePtr->StartOfAllocatedNodes;
	AllocNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr + AllocNodeOffset);

	while (handle != AllocNodePtr->BufferHandle) {
		if (AllocNodePtr->NextNodeOffset == 0) {
			Status = AGESA_BOUNDS_CHK;
			break;
		}
		AllocNodeOffset = AllocNodePtr->NextNodeOffset;
		AllocNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr +
						    AllocNodeOffset);
	}
	*last_allocd_or_match = AllocNodePtr;
	return Status;
}

/*
 * Name			ConcatenateNodes
 * Brief description	Concatenates two adjacent nodes into a single node,
 *			this procedure is used by agesa_DeallocateBuffer().
 * Input parameters
 *	FirstNodePtr	This node is in the front, its header will be
 *			maintained.
 *	SecondNodePtr	This node is in the back, its header will be cleared.
 */
static void ConcatenateNodes(BIOS_BUFFER_NODE *FirstNodePtr,
				BIOS_BUFFER_NODE *SecondNodePtr)
{
	FirstNodePtr->BufferSize += SecondNodePtr->BufferSize +
						sizeof(BIOS_BUFFER_NODE);
	FirstNodePtr->NextNodeOffset = SecondNodePtr->NextNodeOffset;

	/* Zero out the SecondNode header */
	memset(SecondNodePtr, 0, sizeof(BIOS_BUFFER_NODE));
}

#if IS_ENABLED(CONFIG_LATE_CBMEM_INIT)
#error "Only EARLY_CBMEM_INIT is supported."
#endif
ROMSTAGE_CBMEM_INIT_HOOK(EmptyHeap)

AGESA_STATUS agesa_AllocateBuffer (UINT32 Func, UINTN Data, VOID *ConfigPtr)
{
	/*
	 * Size variables explanation:
	 * FreedNodeSize	- the size of the buffer node being examined,
	 *			will be copied to BestFitNodeSize if the node
	 *			is selected as a possible best fit.
	 * BestFitNodeSize	- the size qf the buffer of the node currently
	 *			considered the best fit.
	 * MinimumSize		- the requested size + sizeof(BIOS_BUFFER_NODE).
	 *			Its the minimum size for the buffer to be broken
	 *			down into 2 nodes, once a node is selected as
	 *			the best fit.
	 */
	UINT32              AvailableHeapSize;
	UINT8               *BiosHeapBaseAddr;
	UINT32              CurrNodeOffset;
	UINT32              PrevNodeOffset;
	UINT32              FreedNodeOffset;
	UINT32              FreedNodeSize;
	UINT32              BestFitNodeOffset;
	UINT32              BestFitNodeSize;
	UINT32              BestFitPrevNodeOffset;
	UINT32              NextFreeOffset;
	UINT32              MinimumSize;
	BIOS_BUFFER_NODE   *CurrNodePtr;
	BIOS_BUFFER_NODE   *FreedNodePtr;
	BIOS_BUFFER_NODE   *BestFitNodePtr;
	BIOS_BUFFER_NODE   *BestFitPrevNodePtr;
	BIOS_BUFFER_NODE   *NextFreePtr;
	BIOS_HEAP_MANAGER  *BiosHeapBasePtr;
	AGESA_BUFFER_PARAMS *AllocParams;
	AGESA_STATUS        Status;

	AllocParams = ((AGESA_BUFFER_PARAMS *)ConfigPtr);
	AllocParams->BufferPointer = NULL;
	MinimumSize = AllocParams->BufferLength + sizeof(BIOS_BUFFER_NODE);

	AvailableHeapSize = BIOS_HEAP_SIZE - sizeof(BIOS_HEAP_MANAGER);
	BestFitNodeSize = AvailableHeapSize; /* init with largest possible */
	BiosHeapBaseAddr = agesa_heap_base();
	BiosHeapBasePtr = (BIOS_HEAP_MANAGER *)BiosHeapBaseAddr;

	if (BiosHeapBasePtr->StartOfAllocatedNodes == 0) {
		/* First allocation */
		CurrNodeOffset = sizeof(BIOS_HEAP_MANAGER);
		CurrNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ CurrNodeOffset);
		CurrNodePtr->BufferHandle = AllocParams->BufferHandle;
		CurrNodePtr->BufferSize = AllocParams->BufferLength;
		CurrNodePtr->NextNodeOffset = 0;
		AllocParams->BufferPointer = (UINT8 *)CurrNodePtr
						+ sizeof(BIOS_BUFFER_NODE);

		/* Update the remaining free space */
		FreedNodeOffset = CurrNodeOffset + CurrNodePtr->BufferSize
						+ sizeof(BIOS_BUFFER_NODE);
		FreedNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ FreedNodeOffset);
		FreedNodePtr->BufferSize = AvailableHeapSize
					- (FreedNodeOffset - CurrNodeOffset)
					- sizeof(BIOS_BUFFER_NODE);
		FreedNodePtr->NextNodeOffset = 0;

		/* Update the offsets for Allocated and Freed nodes */
		BiosHeapBasePtr->StartOfAllocatedNodes = CurrNodeOffset;
		BiosHeapBasePtr->StartOfFreedNodes = FreedNodeOffset;
	} else {
		/*
		 * Find out whether BufferHandle has been allocated on the heap.
		 * If it has, return AGESA_BOUNDS_CHK.
		 */
		Status = FindAllocatedNode(AllocParams->BufferHandle,
						&CurrNodePtr);
		if (Status == AGESA_SUCCESS)
			return AGESA_BOUNDS_CHK;

		/*
		 * If status ditn't returned AGESA_SUCCESS, CurrNodePtr here
		 * points to the end of the allocated nodes list.
		 */

		/* Find the node that best fits the requested buffer size */
		FreedNodeOffset = BiosHeapBasePtr->StartOfFreedNodes;
		PrevNodeOffset = FreedNodeOffset;
		BestFitNodeOffset = 0;
		BestFitPrevNodeOffset = 0;
		while (FreedNodeOffset != 0) {
			FreedNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ FreedNodeOffset);
			FreedNodeSize = FreedNodePtr->BufferSize;
			if (FreedNodeSize >= MinimumSize) {
				if (BestFitNodeOffset == 0) {
					/*
					 * First node that fits the requested
					 * buffer size
					 */
					BestFitNodeOffset = FreedNodeOffset;
					BestFitPrevNodeOffset = PrevNodeOffset;
					BestFitNodeSize = FreedNodeSize;
				} else {
					/*
					 * Find out whether current node is a
					 * betterfit than the previous nodes
					 */
					if (BestFitNodeSize > FreedNodeSize) {

						BestFitNodeOffset =
							FreedNodeOffset;
						BestFitPrevNodeOffset =
							PrevNodeOffset;
						BestFitNodeSize = FreedNodeSize;
					}
				}
			}
			PrevNodeOffset = FreedNodeOffset;
			FreedNodeOffset = FreedNodePtr->NextNodeOffset;
		} /* end of while loop */

		if (BestFitNodeOffset == 0) {
			/*
			 * If we could not find a node that fits the requested
			 * buffer size, return AGESA_BOUNDS_CHK.
			 */
			return AGESA_BOUNDS_CHK;
		}

		BestFitNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
					+ BestFitNodeOffset);
		BestFitPrevNodePtr = (BIOS_BUFFER_NODE *) (BiosHeapBaseAddr +
							BestFitPrevNodeOffset);

		/*
		 * If BestFitNode is larger than the requested buffer,
		 * fragment the node further
		 */
		if (BestFitNodePtr->BufferSize > MinimumSize) {
			NextFreeOffset = BestFitNodeOffset + MinimumSize;
			NextFreePtr = (BIOS_BUFFER_NODE *) (BiosHeapBaseAddr +
				       NextFreeOffset);
			NextFreePtr->BufferSize = BestFitNodeSize - MinimumSize;

			/* Remove BestFitNode from list of Freed nodes */
			NextFreePtr->NextNodeOffset =
					BestFitNodePtr->NextNodeOffset;
		} else {
			/*
			 * Otherwise, next free node is NextNodeOffset of
			 * BestFitNode. Remove it from list of Freed nodes.
			 */
			NextFreeOffset = BestFitNodePtr->NextNodeOffset;
		}

		/*
		 * If BestFitNode is the first buffer in the list, then
		 * update StartOfFreedNodes to reflect new free node.
		 */
		if (BestFitNodeOffset == BiosHeapBasePtr->StartOfFreedNodes)
			BiosHeapBasePtr->StartOfFreedNodes = NextFreeOffset;
		else
			BestFitPrevNodePtr->NextNodeOffset = NextFreeOffset;

		/* Add BestFitNode to the list of Allocated nodes */
		CurrNodePtr->NextNodeOffset = BestFitNodeOffset;
		BestFitNodePtr->BufferSize = AllocParams->BufferLength;
		BestFitNodePtr->BufferHandle = AllocParams->BufferHandle;
		BestFitNodePtr->NextNodeOffset = 0;

		AllocParams->BufferPointer = (UINT8 *)BestFitNodePtr +
					     sizeof(BIOS_BUFFER_NODE);
	}

	return AGESA_SUCCESS;
}

AGESA_STATUS agesa_DeallocateBuffer (UINT32 Func, UINTN Data, VOID *ConfigPtr)
{

	UINT8               *BiosHeapBaseAddr;
	UINT32              AllocNodeOffset;
	UINT32              PrevNodeOffset;
	UINT32              NextNodeOffset;
	UINT32              FreedNodeOffset;
	UINT32              EndNodeOffset;
	BIOS_BUFFER_NODE   *AllocNodePtr;
	BIOS_BUFFER_NODE   *PrevNodePtr;
	BIOS_BUFFER_NODE   *FreedNodePtr;
	BIOS_BUFFER_NODE   *NextNodePtr;
	BIOS_HEAP_MANAGER  *BiosHeapBasePtr;
	AGESA_BUFFER_PARAMS *AllocParams;

	AllocParams = (AGESA_BUFFER_PARAMS *)ConfigPtr;

	BiosHeapBaseAddr = agesa_heap_base();
	BiosHeapBasePtr = (BIOS_HEAP_MANAGER *)BiosHeapBaseAddr;

	/* Find target node to deallocate in list of allocated nodes.
	 * Return AGESA_BOUNDS_CHK if the BufferHandle is not found.
	 */
	AllocNodeOffset = BiosHeapBasePtr->StartOfAllocatedNodes;
	AllocNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr + AllocNodeOffset);
	PrevNodeOffset = AllocNodeOffset;

	while (AllocNodePtr->BufferHandle != AllocParams->BufferHandle) {
		if (AllocNodePtr->NextNodeOffset == 0)
			return AGESA_BOUNDS_CHK;
		PrevNodeOffset = AllocNodeOffset;
		AllocNodeOffset = AllocNodePtr->NextNodeOffset;
		AllocNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ AllocNodeOffset);
	}

	/* Remove target node from list of allocated nodes */
	PrevNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr + PrevNodeOffset);
	PrevNodePtr->NextNodeOffset = AllocNodePtr->NextNodeOffset;

	/* Zero out the buffer, and clear the BufferHandle */
	memset((UINT8 *)AllocNodePtr + sizeof(BIOS_BUFFER_NODE), 0,
		AllocNodePtr->BufferSize);
	AllocNodePtr->BufferHandle = 0;

	/* Add deallocated node in order to the list of freed nodes */
	FreedNodeOffset = BiosHeapBasePtr->StartOfFreedNodes;
	FreedNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr + FreedNodeOffset);

	EndNodeOffset = AllocNodeOffset + AllocNodePtr->BufferSize +
						sizeof(BIOS_BUFFER_NODE);

	if (AllocNodeOffset < FreedNodeOffset) {
		/* Add to the start of the freed list */
		if (EndNodeOffset == FreedNodeOffset) {
			/* If the freed node is adjacent to the first node in
			 * the list, concatenate both nodes
			 */
			ConcatenateNodes(AllocNodePtr, FreedNodePtr);
		} else {
			/* Otherwise, add freed node to the start of the list
			 * Update NextNodeOffset and BufferSize to include the
			 * size of BIOS_BUFFER_NODE.
			 */
			AllocNodePtr->NextNodeOffset = FreedNodeOffset;
		}
		/* Update StartOfFreedNodes to the new first node */
		BiosHeapBasePtr->StartOfFreedNodes = AllocNodeOffset;
	} else {
		/* Traverse list of freed nodes to find where the deallocated
		 * node should be placed.
		 */
		NextNodeOffset = FreedNodeOffset;
		NextNodePtr = FreedNodePtr;
		while (AllocNodeOffset > NextNodeOffset) {
			PrevNodeOffset = NextNodeOffset;
			if (NextNodePtr->NextNodeOffset == 0)
				break;
			NextNodeOffset = NextNodePtr->NextNodeOffset;
			NextNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ NextNodeOffset);
		}

		/* If deallocated node is adjacent to the next node,
		 * concatenate both nodes.
		 */
		if (NextNodeOffset == EndNodeOffset) {
			NextNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ NextNodeOffset);
			ConcatenateNodes(AllocNodePtr, NextNodePtr);
		} else {
			/*AllocNodePtr->NextNodeOffset =
			 * 			FreedNodePtr->NextNodeOffset; */
			AllocNodePtr->NextNodeOffset = NextNodeOffset;
		}
		/*
		 * If deallocated node is adjacent to the previous node,
		 * concatenate both nodes.
		 */
		PrevNodePtr = (BIOS_BUFFER_NODE *)(BiosHeapBaseAddr
						+ PrevNodeOffset);
		EndNodeOffset = PrevNodeOffset + PrevNodePtr->BufferSize +
						sizeof(BIOS_BUFFER_NODE);

		if (AllocNodeOffset == EndNodeOffset)
			ConcatenateNodes(PrevNodePtr, AllocNodePtr);
		else
			PrevNodePtr->NextNodeOffset = AllocNodeOffset;
	}
	return AGESA_SUCCESS;
}

AGESA_STATUS agesa_LocateBuffer (UINT32 Func, UINTN Data, VOID *ConfigPtr)
{
	BIOS_BUFFER_NODE    *AllocNodePtr;
	AGESA_BUFFER_PARAMS *AllocParams;
	AGESA_STATUS        Status;

	AllocParams = (AGESA_BUFFER_PARAMS *)ConfigPtr;

	Status = FindAllocatedNode(AllocParams->BufferHandle, &AllocNodePtr);

	if (Status == AGESA_SUCCESS) {
		AllocParams->BufferPointer = (UINT8 *)((UINT8 *)AllocNodePtr
						+ sizeof(BIOS_BUFFER_NODE));
		AllocParams->BufferLength = AllocNodePtr->BufferSize;
	}

	return Status;

}
