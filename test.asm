.model tiny 
.code 
.386
org 100h 
start: 
 jmp dalej
  d	   equ dword ptr
  include memory.inc  
  include vesa.inc
 dalej:
	mov ax,cs
	mov es,ax
	mov ds,ax    
		
 
	call checkXMSinstaled
	cmp ax, 1
	je .xmsInstalled
	lea si, error1
	call printf
	jmp .exit_app
  
.xmsInstalled:  
	call initializeXMS
	call memoryRaport
	
	lea dx, BMP_NAME
	lea esi, IMAGE
	call LoadBmp
	
	call memoryRaport
	
	xor ax, ax
	int 16h
	
	call Start111h	
    
juliaLoop:  
	
	lea esi, SCREEN_BUFFER
	mov ax, 0h
	call FILL
	
	
	lea esi, IMAGE
	lea edi, SCREEN_BUFFER
	mov eax, 100
	xor ebx, ebx
	mov bx, Y_POS
	call PutBitmap	
	
	call blit
	
	mov ax, Y_POS
	add ax, Y_POS_DIR
	mov Y_POS, ax
	
	cmp ax, 200
	jge .neg_dir
	cmp ax, 2	
	jle .neg_dir
	jmp .skip_1
	.neg_dir:
		mov ax, Y_POS_DIR
		neg ax
		mov Y_POS_DIR, ax
	.skip_1:

	mov ah, 1
	int 16h
jz juliaLoop

	lea esi, IMAGE	
	call FreeImg
	call memoryRaport
    
  call StartTxt
  
.exit_app:
 mov ah,4ch
 int 21h

 HANDLER_1 DW ?
 HANDLER_2 DW ?
 
 Y_POS DW 10
 Y_POS_DIR DW 2
 
 IMAGE DB 16 dup(0)
 
 BMP_NAME DB "test.bmp",0
 
 TOTAL_FREE_MEM DB 13, 10, "Total free mem(kb) : ", 0
 LARGEST_FREE_MEM_BLOCK DB "Largest free mem block(kb) : ", 0
 ALLOC_FAIL_MSG DB "Fail to alloc memory", 0
 MOVE_FAIL_MSG DB "Fail to move memory", 0
OK     DB 13,10,"Jest XMS!!!",13,10,0
error1 DB 13,10,"Brak XMS!!!!!",0

NEW_LINE  DB 13,10, 0

SPACE_BAR  DB ' ', 0

checkMoveError:
	cmp ax, 01
	je .success1
	lea si, MOVE_FAIL_MSG
	call printf
	call newLine
	jmp .exit_app
	.success1:
ret	

checkAllocError:
	cmp ax, 01
	je .success
	lea si, ALLOC_FAIL_MSG
	call printf
	call newLine
	jmp .exit_app
	.success:
ret	

memoryRaport:
	lea si, TOTAL_FREE_MEM
	call printf;
	call totalFreeMem
	xor dx, dx
	call printNo
	call newLine
	lea si, LARGEST_FREE_MEM_BLOCK
	call printf
	call largestFreeMemBlock
	xor dx, dx
	call printNo
	call newLine
ret	

newLine:
	lea si, NEW_LINE
	call printf
ret

spaceBar:
	lea si, SPACE_BAR
	call printf
ret

printf:
  lodsb
  or al,al
  jz pdon
  mov ah, 0Eh
  mov bh, 00h
  mov bl, 07h
  int 10h
jmp printf
pdon:
ret

; dx:ax <-liczba
printNo:
 push edx
 push ecx
 xor edx, edx
 lea edi, wynik
 xor bl,bl
 mov [edi],bl
 inc edi
 pow:
 mov ecx,10
 div ecx
 add dl, 48
 mov [edi], dl
 inc edi
 xor edx,edx
 cmp eax,0
 jne pow  
  mov esi, edi
  dec esi
  mov al,[esi]
 ppp:
   mov ah, 0Eh
   mov bh, 00h
   mov bl, 07h
   int 10h
   dec esi
   mov al,[esi]
  cmp al,0
 jne ppp
 pop ecx
 pop edx
ret


wynik  db 20 dup(?)
end start