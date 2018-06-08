.model tiny 
.code 
.386
org 100h 
start: 
 jmp dalej
	d	   equ dword ptr
  
  RUN_SPEED_X equ 256 * 5
  RUN_SPEED_Y equ 192 * 5
  
	STANDING equ 1
	RUNNING  equ 2

	SOUTH equ 0
	WEST  equ 1 
	NORTH equ 2
	EAST  equ 3

	KEY_W 	 equ 17
	KEY_A    equ 30
	KEY_S    equ 31
	KEY_D 	 equ 32
	
	MOUSE_BMP db 16 dup(0)

  include memory.inc  
  include vesa.inc
  include mouse.inc
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
	
	lea dx, BMP_BACKGROUND_NAME
	lea esi, BACKGROUND
	call LoadBmp
	
	lea dx, MOUSE_BMP_NAME
	lea esi, MOUSE_BMP
	call LoadBmp
	
	lea esi, MOUSE_BMP
	call initMouse
	
	call memoryRaport
	
	xor ax, ax
	int 16h
	
	call Start111h	
    
juliaLoop:  
	
	lea esi, SCREEN_BUFFER
	mov ax, 0FFFFh
	call FILL
	
	lea esi, BACKGROUND
	lea edi, SCREEN_BUFFER
	xor eax, eax
	xor ebx, ebx
	call PutBitmap

	call updatAnimationOnKeys	

		xor eax, eax
		mov ax, ANIM_ITER
		shr ax, 10
		mov ebx, SPRITE_W
		mul ebx				
		
		mov ecx, eax   ; ECX anim iter offset
		
		xor edi, edi
		mov di, ANIMATION		
		mov eax, [di + 4]		; eax anim Y pos
		mov ebx, SPRITE_H
		mul ebx					; eax = anim Y pos * Height		
		
		mov ebx, POS_X
		mov edx, POS_Y
		shr ebx, 10
		shr edx, 10
		
		PutPartSprite IMAGE, SCREEN_BUFFER, ecx, eax, ebx, edx, SPRITE_W, SPRITE_H, 0F81Fh	
		
		xor edi, edi
		mov di, ANIMATION
		mov edx, [di]		; edx max anim
		shl edx, 10
		mov ax, ANIM_ITER
		mov bx, ANIM_SPEED
		
		mov ecx, STATE
		cmp ecx, RUNNING
		je running_speed
			shr bx, 1
		running_speed:
		add ax, bx
		cmp ax, dx
		jle anim_in_range
			xor ax, ax
		anim_in_range:
		mov ANIM_ITER, ax
	
	call ShowMouse
	
	call vsync
	call blit
	
	xor ax, ax
	in al,60h
	cmp al,1d
	je bye
	push ax
		mov ah,1
		int 16h 
		jz dda	
			xor ax, ax
			int 16h 
		dda:
	pop ax 	
	mov bl, 1
	cmp ax, 128
	jle enableKey
	  sub ax, 128
	  mov bl, 0  
	enableKey:	  
	  xor edi, edi
	  lea di, keys
	  add di, ax
	  mov [di], bl

	endKeyboardHandler:
	
jmp juliaLoop

bye:
	lea esi, IMAGE	
	call FreeImg
	
	lea esi, BACKGROUND	
	call FreeImg
	
	lea esi, MOUSE_BMP
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
 BACKGROUND DB 16 dup(0)
 
 BMP_NAME DB "anim3.bmp",0
 BMP_BACKGROUND_NAME DB "bk.bmp",0
 MOUSE_BMP_NAME DB "mouse.bmp",0
 
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
	pushad
	lea si, NEW_LINE
	call printf
	popad
ret

spaceBar:
	pushad
	lea si, SPACE_BAR
	call printf
	popad
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
 pushad 
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
 popad
ret

updatAnimationOnKeys:
	pushad

	mov ebx, DIR
	xor edi, edi
	lea di, keys
	xor al, al
	add al, [di + KEY_W]
	add al, [di + KEY_S]
	add al, [di + KEY_A]
	add al, [di + KEY_D]
	
	cmp al, 0
	jz not_running
		mov al, [di + KEY_A]
		cmp al, 1
		jne check_d
		 ; handle A
			mov edx, WEST
			mov DIR, edx
			
			mov edx, POS_X
			sub edx, RUN_SPEED_X
			mov POS_X, edx
			
		jmp run_handled 
		
		check_d:
		mov al, [di + KEY_D]
		cmp al, 1
		jne check_w
			; handle D
			mov edx, EAST
			mov DIR, edx
			
			mov edx, POS_X
			add edx, RUN_SPEED_X
			mov POS_X, edx
			
		jmp run_handled 
		
		check_w:
		mov al, [di + KEY_W]
		cmp al, 1
		jne check_s
		 ; handle W
			mov edx, NORTH
			mov DIR, edx
			
			mov edx, POS_Y
			sub edx, RUN_SPEED_Y
			mov POS_Y, edx
			
		jmp run_handled 
		
		check_s:
		mov al, [di + KEY_S]
		cmp al, 1
		jne check_s
			mov edx, SOUTH
			mov DIR, edx
			
			mov edx, POS_Y
			add edx, RUN_SPEED_Y
			mov POS_Y, edx
			
		jmp run_handled 
		
		run_handled:		
		
			mov edx, STATE
			mov ecx, RUNNING
			mov STATE, ecx											
			cmp edx, ecx       ; edx old state, ecx new state
			jne update_animation ; ebx - old DIR
			cmp ebx, DIR			
			jne update_animation
			
		
	jmp end_handler
	not_running:		
		mov eax, STATE		
		mov edx, STANDING
		mov STATE, edx
							
		cmp eax, STANDING
		je end_handler   ; was standing and still standing
		
		; was moving, now standing
			
		mov eax, STANDING
		mov STATE, eax
		jmp update_animation
		
	
		jmp end_handler
		
		update_animation:
			mov edx, STATE
			dec edx
			shl edx, 3			; (STATE-1) * 8
			
			mov eax, DIR
			shl eax, 1			; DIR * 2
			
			lea esi, ANIMATION_ARRAY
			add esi, eax
			add esi, edx
			
			mov ax, [esi]   ; animation offset 
			mov ANIMATION, ax
			xor ax, ax
			mov ANIM_ITER, ax
			
	end_handler:
	popad
ret

SPRITE_W DD 120
SPRITE_H DD 130


SOUTH_STAND DD 3, 0  ; 3 animations, y start = 0
WEST_STAND  DD 3, 1  ; 3 animations, y start = 1
NORTH_STAND DD 1, 2  ; 1 animations, y start = 2
EAST_STAND  DD 3, 3  ; 3 animations, y start = 3

SOUTH_RUN DD 10, 4  ; 10 animations, y start = 4
WEST_RUN  DD 10, 5  ; 10 animations, y start = 5
NORTH_RUN DD 10, 6  ; 10 animations, y start = 6
EAST_RUN  DD 10, 7  ; 10 animations, y start = 7
			
ANIMATION_ARRAY DW offset SOUTH_STAND, offset WEST_STAND, offset NORTH_STAND, offset EAST_STAND
				DW offset SOUTH_RUN, offset WEST_RUN, offset NORTH_RUN, offset EAST_RUN

STATE DD STANDING
DIR   DD WEST

ANIMATION DW offset WEST_STAND
ANIM_ITER DW 0
ANIM_SPEED DW 32 * 5

keys db 256 dup(0)
			
wynik  db 20 dup(?)

POS_X DD 320 * 1024
POS_Y DD 240 * 1024

end start