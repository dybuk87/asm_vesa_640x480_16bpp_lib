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
	
	; eax = 1024*768*4  try alloc mem for image 1024x768 32bits ~ 3mb
	mov eax, 1024 * 4 * 768	
	lea	di, HANDLER_1
	call allocateMem
	call checkAllocError
	call memoryRaport	
	
	; second alloc 
	mov eax, 1024 * 4 * 768	
	lea	di, HANDLER_2
	call allocateMem
	call checkAllocError
	call memoryRaport
	
	MoveExtMem HANDLER_1,0,HANDLER_2,0, 1024*4*768
	
	lea si, IMAGE	
	mov edi, 640
	mov eax, 480
	call NewImg
	call checkAllocError
	call memoryRaport
	
	lea si, IMAGE
	call freeImg
	call memoryRaport
	
	mov dx, HANDLER_1
	call freeMem		
	call memoryRaport
	
	mov dx, HANDLER_2
	call freeMem		
	call memoryRaport

  xor ax,ax
  int 16h
  
  call Start111h
  
  lea si, SCREEN_BUFFER
  mov ax, 0h
  call FILL
  
  finit
  
juliaLoop:  
	fld  d k1
	fadd d ak1
	fld  d k2
	fadd d ak2
	fst  d k2
	fst  d k1
	fsin
	fmul d l1024 
	fistp d cxx
	fcos
	fmul  d l1024
	fistp d cyy
  
  
	call julia
	call BLIT

	mov ah, 1
	int 16h
jz juliaLoop
  
  xor ax,ax
  int 16h
  
  call StartTxt
  
.exit_app:
 mov ah,4ch
 int 21h

 HANDLER_1 DW ?
 HANDLER_2 DW ?
 
 IMAGE DB 16 dup(0)
 
 TOTAL_FREE_MEM DB 13, 10, "Total free mem(kb) : ", 0
 LARGEST_FREE_MEM_BLOCK DB "Largest free mem block(kb) : ", 0
 ALLOC_FAIL_MSG DB "Fail to alloc memory", 0
 MOVE_FAIL_MSG DB "Fail to move memory", 0
OK     DB 13,10,"Jest XMS!!!",13,10,0
error1 DB 13,10,"Brak XMS!!!!!",0

NEW_LINE  DB 13,10, 0

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
 push dx
 push cx
 xor dx,dx
 lea di, wynik
 xor bl,bl
 mov [di],bl
 inc di
 pow:
 mov cx,10
 div cx
 add dl,48
 mov [di],dl
 inc di
 xor dx,dx
 cmp ax,0
 jne pow  
  mov si,di
  dec si
  mov al,[si]
 ppp:
   mov ah, 0Eh
   mov bh, 00h
   mov bl, 07h
   int 10h
   dec si
   mov al,[si]
  cmp al,0
 jne ppp
 pop cx
 pop dx
ret

julia:
	xor eax, eax
	mov out_index, eax
	push es
	mov ax, 09000h
	mov es, ax
	mov ax, -2000
	mov j, ax
	xor di, di

	@petY:
		mov ax, -1920
		mov i, ax 

		@petX:  
			xor 	cx,	cx
			movsx	eax,	i
			movsx	ebx,	j
			mov		x,	eax
			mov		y,	ebx
			imul	eax
			mov		x2,	eax
			xchg	eax,	ebx
			imul	eax
			mov		y2,	eax
			add		eax,	ebx
			jc		koniec_while
			while_:
				cmp 	eax,	4*1024*1024
				jge	koniec_while	
				cmp	cx,	31
				jz		koniec_while

				inc cx
				mov	eax,	x
				imul	d y
				sar	eax,	9
				add	eax,	cyy
				mov	y,	eax
				mov	eax,	x2
				sub	eax,	y2
				sar	eax,	10
				add	eax,	cxx
				mov	x,	eax
				imul	eax
				mov	x2,	eax
				mov	eax,	y
				imul	eax
				mov	y2,	eax
				add	eax,	x2
				jc		koniec_while
			jmp while_
			koniec_while:								
				shl cx, 12
				mov es:[di], cx
				inc di								
				inc di
				cmp di, 0
				jne bufOk								
					push eax
					push si
					push es
					push ds
					mov ax, cs
					mov es, ax
					mov ds, ax
					lea si, SCREEN_BUFFER
					add	si, HANDLE_
					MoveExtMem 0, 090000000h, [si], out_index, 0ffffh	
					mov eax, out_index
					add eax, 010000h
					mov out_index, eax
					pop ds
					pop es
					pop si
					pop eax				
					xor di, di
			bufOk:	
			mov ax, i
			add ax, 6
			mov i, ax
			cmp ax, 1920
		jl @petX 
		mov ax, j
		add ax, 8
		mov j, ax
		cmp ax, 2000
	jl @petY
	pop es
ret

k1      DD 0.0
k2      DD 0.0
ak1     DD 0.01
ak2     DD 0.015

out_index DD 0

l1024 dd 900.0

 x dd ?
 y dd ?
x2 dd ?
y2 dd ?
 i dw ?
 j dw ?

cxx dd 0
cyy dd 0

wynik  db 20 dup(?)
end start