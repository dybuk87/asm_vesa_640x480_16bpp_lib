;-----------------------------------------
;   WE: Brak
;   WY: BX - button state; CX - X; DX - Y
; info: Get mouse position and button state
;-----------------------------------------
MousePos:	
	push ax
	mov ax, 03
	int 33h
	push cx
	
	mov ax, dx
	mov cx, 480
	mul cx
	mov cx, 200
	div cx
	
	mov dx, ax
	
	pop cx
	pop ax
ret

MOUSE_TIMAGE dd 0

; SI - TIMAGE structure for mouse pointer
initMouse:
	mov MOUSE_TIMAGE, ESI
ret


ShowMouse:
	push ds
	push es
	pushad	 
	 mov esi, MOUSE_TIMAGE
	 cmp esi, 0
	 je .no_mouse_img
	 
		xor ecx, ecx
		xor edx, edx
		call MousePos
		
		lea edi, SCREEN_BUFFER
		mov esi, MOUSE_TIMAGE
		 
		push dword ptr 0
		push dword ptr 0
		push ecx
		push edx
		push dword ptr 32
		push dword ptr 32
		push word ptr 0F81Fh
		call PutPartSpriteProc 
		
	
	.no_mouse_img:	
	popad
	pop es
	pop ds
ret
