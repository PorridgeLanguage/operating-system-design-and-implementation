#include "klib.h"
#include "vme.h"
#include "proc.h"

static TSS32 tss;

void init_gdt() {
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_USER);
  gdt[SEG_TSS]   = SEG16(STS_T32A,     &tss,  sizeof(tss)-1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0) {
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;

static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used));

typedef union free_page {
    union free_page *next;
    char buf[PGSIZE];
} page_t;

static page_t *free_page_list = NULL;


// WEEK3-virtual-memory

void init_page() {
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");


  // WEEK3-virtual-memory: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
  // TODO();
  
  for (int i = 0; i < PHY_MEM / PT_SIZE; i++) {
    kpd.pde[i].val = MAKE_PDE(&kpt[i], 1);
    for (int j = 0; j < NR_PTE; j++) {
      uint32_t addr = (i << DIR_SHIFT) | (j << TBL_SHIFT);
      kpt[i].pte[j].val = MAKE_PTE(addr, 1);
    }
  }

  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);

  // WEEK3-virtual-memory: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  // TODO();
  free_page_list = (void*)KER_MEM;
  for (uint32_t addr = (size_t)free_page_list; addr < PHY_MEM; addr += PGSIZE) {
    page_t *page = (page_t *)addr;
    page->next = free_page_list;
    free_page_list = page;
  }
}

void *kalloc() {
  // WEEK3-virtual-memory: alloc a page from kernel heap, abort when heap empty
  // TODO();
  if (free_page_list == NULL) {
    assert(0);
  }

  page_t *page = free_page_list;

  if (page->next == NULL) {
    return 0;
  }

  free_page_list = free_page_list->next;
  memset(page, 0x0, PGSIZE);
  return page;
}

void kfree(void *ptr) {
  // WEEK3-virtual-memory: free a page to kernel heap
  // you can just do nothing :)
  // TODO();
}

PD *vm_alloc() {
  // WEEK3-virtual-memory: alloc a new pgdir, map memory under PHY_MEM identityly
  // TODO();
  PD *pgdir = (PD *)kalloc();
  if (pgdir == NULL) {
    return NULL;
  }

  for (int i = 0; i < PHY_MEM / PT_SIZE; i++) {
    pgdir->pde[i].val = MAKE_PDE(&kpt[i], PTE_W | PTE_P);
  }

  for (int i = PHY_MEM / PT_SIZE; i < NR_PDE; i++) {
    pgdir->pde[i].val = 0;
  }
  return pgdir;
}

void vm_teardown(PD *pgdir) {
  // WEEK3-virtual-memory: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  // TODO();

}

PD *vm_curr() {
  return (PD*)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot) {
  // WEEK3-virtual-memory: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  // TODO
  assert((prot & ~7) == 0);
  int pd_index = ADDR2DIR(va);
  PDE *pde = &(pgdir->pde[pd_index]);
  if(!pde->present){
    if(prot&1){
      PT *new_pt = (PT *)kalloc(); // alloc PT
      memset(new_pt, 0x00, PGSIZE);
      pgdir->pde[pd_index].val |= MAKE_PDE(new_pt, prot); // let pde's prot |= prot
      int pt_index = ADDR2TBL(va);
      PTE *pte = &(new_pt->pte[pt_index]);
      return pte;
    } else {
      return NULL;
    }
  }
  PT* pt = PDE2PT(*pde);
  int pt_index = ADDR2TBL(va);
  PTE *pte = &(pt->pte[pt_index]);
  return pte;
}

void *vm_walk(PD *pgdir, size_t va, int prot) {
  // WEEK3-virtual-memory: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  // TODO();
  PTE *pte = vm_walkpte(pgdir, va, prot);
  if (prot&1 && ((pte->val & prot & 7) != prot)) {
    vm_pgfault(va, 2);
  }
  if (pte == NULL && !(prot&1)) {
    return NULL;
  }
  void *page = PTE2PG(*pte);
  return (void*)((uint32_t)page | ADDR2OFF(va));
}



void vm_map(PD *pgdir, size_t va, size_t len, int prot) {
  // WEEK3-virtual-memory: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(end >= start);

  // TODO();
  for(int i = start; i < end; i += PGSIZE){
    PTE* pte = vm_walkpte(pgdir, i, prot);
    if (!pte->present) {
      PTE* new_pte = (PTE *)kalloc();
      pte->val = MAKE_PTE(new_pte, prot);
    } else {
      // just let pte->prot |= prot
      pte->val |= prot;
    }
  }
}

void vm_unmap(PD *pgdir, size_t va, size_t len) {
  // WEEK3-virtual-memory: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  // assert(ADDR2OFF(va) == 0);
  // assert(ADDR2OFF(len) == 0);
  // TODO();
  
}

void vm_copycurr(PD *pgdir) {
  // WEEK4-process-api: copy memory mapped in curr pd to pgdir
  TODO();
}

void vm_pgfault(size_t va, int errcode) {
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
