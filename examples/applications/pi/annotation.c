
#include <stdio.h>

typedef struct {
  uint32_t          Type;
  uint32_t          InstructionCount;
  uint32_t          CycleCount;
  uint32_t          LoadCount;
  uint32_t          StoreCount;
} annotation_db_t;

void mbb_annotation(annotation_db_t *db)
{
    __asm__ volatile(
        "   mov    $0x4000,%dx\n\t"
        "   out    %eax,(%dx)"
    ); 

    //printf("@db: 0x%08x", db); 
    //db = 0; 
    //eu_base::get_current_eu()->annotate(db);
}

/*
00100010 <mbb_annotation>:
  100010:       83 ec 04                sub    $0x4,%esp
  100013:       8b 44 24 08             mov    0x8(%esp),%eax
  100017:       89 04 24                mov    %eax,(%esp)

//  10001d:       66 ba 00 50             mov    $0x5000,%dx
//  100021:       ef                      out    %eax,(%dx)

  100022:       83 c4 04                add    $0x4,%esp
  100025:       c3                      ret    
*/
