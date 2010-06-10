/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H
#define KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H


#include <KernelExport.h>

#include "paging/pae/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


#if B_HAIKU_PHYSICAL_BITS == 64


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;


class X86PagingMethodPAE : public X86PagingMethod {
public:
								X86PagingMethodPAE();
	virtual						~X86PagingMethodPAE();

	virtual	status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	virtual	status_t			InitPostArea(kernel_args* args);

	virtual	status_t			CreateTranslationMap(bool kernel,
									VMTranslationMap** _map);

	virtual	status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									phys_addr_t (*get_free_page)(kernel_args*));

	virtual	bool				IsKernelPageAccessible(addr_t virtualAddress,
									uint32 protection);

	inline	X86PhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	inline	pae_page_directory_entry* const* KernelVirtualPageDirs() const
									{ return fKernelVirtualPageDirs; }

	static	X86PagingMethodPAE*	Method();

	static	void				PutPageTableInPageDir(
									pae_page_directory_entry* entry,
									phys_addr_t physicalTable,
									uint32 attributes);
	static	void				PutPageTableEntryInTable(
									pae_page_table_entry* entry,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									bool globalPage);

	static	pae_page_directory_entry* PageDirEntryForAddress(
									pae_page_directory_entry* const* pdpt,
									addr_t address);

	static	uint32				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);

private:
			struct ToPAESwitcher;
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
			bool				_EarlyQuery(addr_t virtualAddress,
									phys_addr_t* _physicalAddress);
			pae_page_table_entry* _EarlyGetPageTable(phys_addr_t address);

private:
			X86PhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;

			void*				fEarlyPageStructures;
			size_t				fEarlyPageStructuresSize;
			pae_page_directory_entry* fKernelVirtualPageDirs[4];
			phys_addr_t			fKernelPhysicalPageDirs[4];
			addr_t				fFreeVirtualSlot;
			pae_page_table_entry* fFreeVirtualSlotPTE;
};


/*static*/ inline X86PagingMethodPAE*
X86PagingMethodPAE::Method()
{
	return static_cast<X86PagingMethodPAE*>(gX86PagingMethod);
}


/*static*/ inline pae_page_directory_entry*
X86PagingMethodPAE::PageDirEntryForAddress(
	pae_page_directory_entry* const* pdpt, addr_t address)
{
	return pdpt[address >> 30]
		+ (address / kPAEPageTableRange) % kPAEPageDirEntryCount;
}


/*static*/ inline uint32
X86PagingMethodPAE::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	// ATM we only handle the uncacheable and write-through type explicitly. For
	// all other types we rely on the MTRRs to be set up correctly. Since we set
	// the default memory type to write-back and since the uncacheable type in
	// the PTE overrides any MTRR attribute (though, as per the specs, that is
	// not recommended for performance reasons), this reduces the work we
	// actually *have* to do with the MTRRs to setting the remaining types
	// (usually only write-combining for the frame buffer).
	switch (memoryType) {
		case B_MTR_UC:
			return X86_PAE_PTE_CACHING_DISABLED | X86_PAE_PTE_WRITE_THROUGH;

		case B_MTR_WC:
			// X86_PTE_WRITE_THROUGH would be closer, but the combination with
			// MTRR WC is "implementation defined" for Pentium Pro/II.
			return 0;

		case B_MTR_WT:
			return X86_PAE_PTE_WRITE_THROUGH;

		case B_MTR_WP:
		case B_MTR_WB:
		default:
			return 0;
	}
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64


#endif	// KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H