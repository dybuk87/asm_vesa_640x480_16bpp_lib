include file.inc

; each bitmap need 16 bytes for handler
;  XMS memory handler : uint16
;  Image Res X        : uint32
;  Image Res Y	      : uint32
;  Image Size in bytes: uint32
;  padding	      : uint16 

; We have opne bitmap for double buffering, memory allocation in Start111h
SCREEN_BUFFER DB 16 dup(0);   

; constant offset for each value in image  handler 
   handle_ equ  0
   ResX_   equ  2
   ResY_   equ  6
   Size_   equ 10

; Allocate memory for new image 16 bit color
; SI - Pointer to 16 bytes handler (it will save Handler, sizeX, sizeY, Size into it) 
; EDI, EAX - Image resolution ResX x ResY
NewImg:
 mov [SI+RESX_],EDI   ; Save bitmap imension
 mov [SI+RESY_],EAX   
 mul edi              ; EDX : EAX = EAX*EDI
 shl eax,1            ; EAX = ResX*resY*BPP   (each color require 2 bytes)
 mov [SI+SIZE_],EAX   ; Save bitmap size in bytes  
 mov di, si
 add di, HANDLE_      ; es:di - pointer where XMS memory handler will be saved
 call allocateMem     
ret   
  
; Free XMS memory allocated for image
; SI - Pointer to 16 byte bitmap handler
FreeImg:
 push si
 mov DX, word ptr [SI+HANDLE_]   ; all we need to free mem is handler
 call freeMem 	
 pop si
 xor dx,dx
 mov [SI+Handle_], dx		; clear handler in image structure
ret   
   

; Initialize vesa graphic 
; 111h - this is id for graphic 640x480 16 bit color   
Start111h:
	;jmp @ok_v
	mov ax,4F02h
	mov bx,111h
	int 10h       ; after this line we should have 640x480 16 bit color enabled
 
	cmp ax,004Fh  ; check if ok
	je @ok_v
		mov ah,4ch
		int 21h 
	@ok_v:
 
	; allocate bitmap for double buffer
	lea si, SCREEN_BUFFER	
	mov edi, 640
	mov eax, 480
	call NewImg
ret 
 

; This method send SCREEN_BUFFER to actual visible buffer using vesa pageing 
; We are working in Real mode, so graphic window is in 0xA000 : 0000x to 0xA000 : 0xFFFF that is why we can access  65536 bytes(65kb) of screen
; this is not enought to show 640x480 16bpp (614400 bytes - 600 kb)
; vesa driver allow to remap this 0xA000 to point different parts of actual screen. 
BLIT:
 lea si, SCREEN_BUFFER   ; source image
 mov eax, 614400 	 ; this is size of screen - 600kb
 xor ecx,ecx		 ; eax will be used to track how many bytes left to transfer from screen_buffer
 xor edx,edx		 ; ebx store information how many bytes to send in each loop (64kb or less if no more data available)
			 ; ecx - source pointer 
			 ; edx - page number
			 ; dest offset in real screen is calculated as follow :
			 ;  offset = PAGE_NUMBER * PAGE_SIZE(65536) + ADRESS_IN_PAGE
			 ; to calculate page and offset in page for pixel we can caluclate 
			 ; base_offset = (x + y * 640) * 2
			 ; page_no = base_offset / PAGE_SIZE
			 ; offset_in_page = base_offset % PAGE_SIZE
			 ; to change pixel we ned to change vesa to page to "page_no" and write 2 bytes to address 0xA000:offset_in_page
 @pet_@:		 ; each loop copy 64kb of data to 0A000
  mov ebx,10000h	
  cmp eax,10000h
  jae ___ok  
   mov ebx,eax
   mov eax,10000h	
  ___ok:
  sub eax,10000h  

	; at this point eax - how many data left to copy, ebx - hom many data to copy 
	; ecx - pointer to source data in screen_buffer

 ; switch vesa page
  push si
  push edx  
  push eax
  push ecx
  
  push ebx
   mov ax,04F05h
   mov bx,0   
   int 10h  ; this interrupt change page, page no in dx
  pop ebx
  
  pushad

; this method send ebx bytes from Image.handle:ecx to 0A000:0000
; XMS use 0 handler to access real mode (application) memory, offset is calculated as  SEGMENT * 65536 + OFFSET 
  moveExtMem [SI+HANDLE_], ecx, 0, 0A0000000h, ebx  
  cmp al,1
  je ooookkkk 
  mov ah,4ch
  int 21h
ooookkkk:
  popad
  pop ecx
  pop eax
  pop edx
  pop si
  add ecx,10000h ; update source pointer
  inc edx        ; inc page number
 cmp eax,0
 jne @pet_@
ret 
 

; Fill whole image with one color
; DS:SI - image handler
; AX - color in RGB_565
FILL:
 ; first fill AX color to line buffer
 lea di, linia
 mov cx,[SI+ResX_]   ; we try to fill line in the same size as image ResX
 rep stosw	

 ; at this point we have line size ResX filled with desired color
 ; now we can send it XMS image buffer ResY times
 
 xor edx, edx
 mov dx,cs
 shl edx,16
 mov dx,offset linia    ; edx - far pointer for XMS  (SEGMENT * 65536 + line offset)

 mov ecx,[SI+ResY_]     ; ecx - how meny scan lines
 xor ebx,ebx
 
 @petlas:
  push esi
  push edx
  push ecx
  push ebx
   mov ecx,[SI+RESX_]
   
   shl ecx,1
   push ecx
   pushad

   ; 0:EDX is pointer to line filled with desired color
   ; SI+HANDLER_ - XMS memory pointer to destination image
   ; ebx - destination position  =  y * ResX where y inc every loop
   ; ecx - size of scan line (image resX)
   MoveExtMem 0, edx, [SI+Handle_], ebx,  ecx        
  
   cmp al,1 
   je ooookk
     mov ah,4ch
    int 21h
  ooookk:
  popad
  pop ecx
  pop ebx
  add ebx,ecx
  pop ecx
  pop edx
  pop esi
 loop @petlas

ret 

; method convert RGB_888(AX, BX, CX) to RG_565 (AX) 
; IN:  AX - Red;
;      BX - Green;
;      CX - Blue;
; OUT: AX - kolor;
RGBToCol:
 shr ax,3
 shl ax,11
 shr bx,2
 shl bx,5
 or ax,bx
 shr cx,3
 or ax,cx 
ret


; Draw part of src bitmap to dest bitmap
; srcX, srcY - this is upper left corner(star position) of src bitmap
; destX, destY - this is destination position to draw bitmap
; sX, sY  - size of copied bitmap
; 
; example if we want to copy part of source bitmap starting from position (150, 90) and size (40, 120), to
; position 100, 20 in destination bitmap we will call this method:
; PutPartBitmap srcBitmap, destBitmap, 150, 90, 100, 20, 40, 120   
PutPartBitmap macro srcHandler, destHandler, srcX, srcY, destX, destY, sX, sY
 push ds
 push es
 pushad
 
 lea esi, srcHandler
 lea edi, destHandler

 push dword ptr srcX
 push dword ptr srcY
 push dword ptr destX
 push dword ptr destY
 push dword ptr sX
 push dword ptr sY
 call PutPartBitmapProc 
 
 popad
 pop es
 pop ds
endm

@sY    equ [EBP +  6]
@sX    equ [EBP + 10]
@destY  equ [EBP + 14]
@destX  equ [EBP + 18]
@srcY  equ [EBP + 22]
@srcX  equ [EBP + 26]

@SKIP_SRC_OFFSET equ [EBP - 4]
@SKIP_DST_OFFSET equ [EBP - 8]

@SRC_HANDLER equ [EBP - 10]
@DST_HANDLER equ [EBP - 12]

@LOOP_X_SIZE equ [EBP - 16]


; this procedure is called by macro PutPartBitmap, for details check PutPartBitmap macro
; ds:si <- image handler
; es:di <- output handler
PutPartBitmapProc:
	push ebp
        mov ebp, esp
	sub esp, 32		; we reserve stack for variables

	; put src and dst bitmap XMS handlers to variables
	xor ax, ax
	mov ax, [ESI + HANDLE_]
	mov @SRC_HANDLER, ax	
	
	xor ax, ax
	mov ax, [EDI + HANDLE_]
	mov @DST_HANDLER, ax
		
	; we will copy line by line, so get line length in bytes and store in variable
	mov eax, @sX
	shl eax, 1			; EAX <- cpy loop size in bytes 2 byte per pixel
	mov ecx, eax 			; EAX i ECX <- cpy loop size
	mov @LOOP_X_SIZE, eax
	
	; after copying one line we have to move to next line, for each bitmap
	; skip bytes equal 2 times ResX for each bitmap, we calculate it and store to variables
	mov eax, [ESI + ResX_]
	shl eax, 1
	mov @SKIP_SRC_OFFSET, eax       
	
	mov eax, [EDI + ResX_]
	shl eax, 1
	mov @SKIP_DST_OFFSET, eax
	
	; calculate start position in bytes :  (srcY * src.ResX + srcX) * 2	
	mov ecx, @srcY
	mov eax, @srcY
	mov ebx, [ESI + ResX_]	
	mul ebx
	add eax, @srcX
	shl eax, 1  		; EAX <- SRC OFFSET
	mov esi, eax		; ESI <- SRC OFFSET

	; calculate destination address in bytes : (dstX * dst.REsX + dstX) * 2	
	mov eax, @destY
	mov ebx, [EDI + ResX_]
	mul ebx
	add eax, @destX
	shl eax, 1			; EAX <- DST OFFSET	
	mov edi, eax		; EDI <- DST OFFSET	
	
	mov eax, 0    ; loop index
	
	mov ecx, @sY  ; bytes to copy 
		
.ppb_y_loop:	
	push eax
	
	pushad

	; copy one line from src to dst	
	moveExtMem  @SRC_HANDLER, esi,  @DST_HANDLER, edi, @LOOP_X_SIZE
	
	popad
	
	
	; shift src and dst index
	add esi, @SKIP_SRC_OFFSET
	add edi, @SKIP_DST_OFFSET
	

	pop eax
	inc eax
loop .ppb_y_loop ; loop until copied sY lines
	
	add esp, 32
	pop ebp
ret 24

; Work exactly like PutPartBitmap but with additional sprColor that allow transparency
; sprColor is transparent color
PutPartSprite macro srcHandler, destHandler, srcX, srcY, destX, destY, sX, sY, sprColor
 push ds
 push es
 pushad
 
 lea esi, srcHandler
 lea edi, destHandler

 push dword ptr srcX
 push dword ptr srcY
 push dword ptr destX
 push dword ptr destY
 push dword ptr sX
 push dword ptr sY
 push word ptr sprColor
 call PutPartSpriteProc 
 
 popad
 pop es
 pop ds
endm

@SPR_COLOR  equ [EBP +  6]
@SPR_sY     equ [EBP +  8]
@SPR_sX     equ [EBP + 12]
@SPR_destY  equ [EBP + 16]
@SPR_destX  equ [EBP + 20]
@SPR_srcY   equ [EBP + 24]
@SPR_srcX   equ [EBP + 28]

@SPR_SKIP_SRC_OFFSET equ [EBP - 4]
@SPR_SKIP_DST_OFFSET equ [EBP - 8]

@SPR_SRC_HANDLER equ [EBP - 10]
@SPR_DST_HANDLER equ [EBP - 12]

@SPR_LOOP_X_SIZE equ [EBP - 16]

; called from PutPartSprite macro, check macro documentation for details
; ds:si <- image handler
; es:di <- output handler
PutPartSpriteProc:
	push ebp
        mov ebp, esp
	sub esp, 32              ; THIS PART WORK EXACTLY LIKE PutPartBitmap

	xor ax, ax
	mov ax, [ESI + HANDLE_]
	mov @SPR_SRC_HANDLER, ax
	
	xor ax, ax
	mov ax, [EDI + HANDLE_]
	mov @SPR_DST_HANDLER, ax
		
	mov eax, @SPR_sX
	shl eax, 1			; EAX <- cpy loop size
	mov ecx, eax 		; EAX i ECX <- cpy loop size
	mov @SPR_LOOP_X_SIZE, eax
	
	mov eax, [ESI + ResX_]
	shl eax, 1
	;sub eax, ecx		;  EAX = 2 * (SRC.RES_X - sX)  ; skip src offset 
	mov @SPR_SKIP_SRC_OFFSET, eax
	
	mov eax, [EDI + ResX_]
	shl eax, 1
	;sub eax, ecx		;  EAX = 2 * (DST.RES_X - sX)  ; skip dst offset 
	mov @SPR_SKIP_DST_OFFSET, eax
	
	mov ecx, @SPR_srcY
	
	
	mov eax, @SPR_srcY
	mov ebx, [ESI + ResX_]	
	mul ebx
	add eax, @SPR_srcX
	shl eax, 1  		; EAX <- SRC OFFSET
	mov esi, eax		; ESI <- SRC OFFSET
	
	mov eax, @SPR_destY
	mov ebx, [EDI + ResX_]
	mul ebx
	add eax, @SPR_destX
	shl eax, 1			; EAX <- DST OFFSET	
	mov edi, eax		; EDI <- DST OFFSET	
	
	mov eax, 0
	
	mov ecx, @SPR_sY
		
.SPR_ppb_y_loop:	
	push eax
	
	pushad

	; THIS COPY LINE LOOP IS DIFFERENT THAN IN COPY BITMAP PART
	; insted from just copying line, we do copy src bitmap line to hline buffer
	; and destination bitmap line to hline2 buffer	
	
	xor ebx, ebx
	mov bx, cs
	shl ebx, 16	
	lea eax,  hline
	add ebx, eax       ; hline offset in XMS memory => cs * 65536 + offset hline	
			   ; copy source line to hline
	moveExtMem  @SPR_SRC_HANDLER, esi,  0, ebx, @SPR_LOOP_X_SIZE
	
	xor ecx, ecx
	mov cx, cs
	shl ecx, 16	
	lea eax,  hline2
	add ecx, eax	   ; hline2 offset in XMS memory => cs * 65536 + offset hline2
			   ; copy destination line to hline2
	moveExtMem  @SPR_DST_HANDLER, edi,  0, ecx, @SPR_LOOP_X_SIZE
	
	push esi
	push edi

	; now we have hline that contain source line, and hline2 that contain destination line
	; at this point we copy each pixel of hline to hline2 when source pixel is different that sprColor
	
	lea esi, hline
	lea edi, hline2
	mov ecx, @SPR_LOOP_X_SIZE
	shr ecx, 1
	.SPR_ppb_x_loop:
		lodsw
		cmp ax, @SPR_COLOR
		je .SPR_ppb_no_copy
			mov [di], ax
		.SPR_ppb_no_copy:
		add di, 2
	loop .SPR_ppb_x_loop

	; hline2 contains merged source line to dest line with excluded sprColors	
	
	pop edi
	pop esi
	
	xor ecx, ecx
	mov cx, cs
	shl ecx, 16	
	lea eax,  hline2
	add ecx, eax
			; last thing left is to copy hline2 to destination
	moveExtMem  0, ecx, @SPR_DST_HANDLER, edi, @SPR_LOOP_X_SIZE
	
	popad
	
	; jump to next line	
	add esi, @SPR_SKIP_SRC_OFFSET
	add edi, @SPR_SKIP_DST_OFFSET
	

	pop eax
	inc eax
	dec ecx
	cmp ecx, 0
jnz .SPR_ppb_y_loop
	
	add esp, 32
	pop ebp
ret 26

; Put whole source bitmap to destination at x,y position
; ds:si <- image handler
; es:di <- output handler
; eax, ebx <- dest pos (x,y)
PutBitmap:
	push bp
	mov bp, sp
	
	push esi
	push edi

	; calc dest point
	xchg ebx, eax				; EAX = Y, EBX = X	
	mov ecx, es:[DI + ResX_]	; EAX = Y, EBX = X  ECX = dst.ResX
					
	add ecx, ecx				; EAX = Y, EBX = X  ECX = dst.ResX * 2
	mul ecx						; EAX = Y * dst.ResX * 2, EBX = X  ECX = dst.ResX * 2
	add eax, ebx
	add eax, ebx				; EAX = dest ptr(x,y),   EBX = X, ECX = 2 * dst.ResX		
	
	mov ebx, ds:[SI + ResX_]	; EAX = dest ptr(x,y),   EBX = src.ResX, ECX = 2 * dst.ResX		
	add ebx, ebx				; EAX = dest ptr(x,y),   EBX = 2 * src.ResX, ECX = 2 * dst.ResX		
			
	
	push eax
	push ebx
	push ecx			;  STACK  : BP ESI EDI (dest ptr(x,y)) (2 * src.ResX) (2 * dst.ResX) 
	
	mov ecx, ds:[SI + ResY_]     ;  STACK  : BP  ESI EDI (dest ptr(x,y))  (2 * src.ResX) (2 * dst.ResX)    |   ecx = ResY_	
			
	xor ebx, ebx
	mov bx, ds:[SI + HANDLE_]
	push ebx
	
	mov bx, es:[DI + HANDLE_]
	push ebx					;  STACK  : BP  ESI EDI (dest ptr(x,y))  (2 * src.ResX) (2 * dst.ResX)  SRC_HANDLER DEST_HANDLER  |   ecx = ResY_	
	
	pop ebx
	pop eax					;  STACK  : BP  ESI EDI (dest ptr(x,y))  (2 * src.ResX) (2 * dst.ResX) |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	
		
	pop edx				 ;  STACK  : BP  ESI EDI (dest ptr(x,y))  (2 * src.ResX)   |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	EDX = (2 * dst.ResX)	
	pop esi				 ;  STACK  : BP  ESI EDI (dest ptr(x,y))       |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	EDX = (2 * dst.ResX)	 ESI = (2 * src.ResX)
	pop edi				 ;  STACK  : BP  ESI EDI      |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	EDX = (2 * dst.ResX)	 ESI = (2 * src.ResX) EDI = (dest ptr(x,y))
	
	push esi				 ;  STACK  : BP  ESI EDI (2 * src.ResX)  |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	EDX = (2 * dst.ResX)	 ESI = (2 * src.ResX) EDI = (dest ptr(x,y))
	
	xor esi, esi				 ;  STACK  : BP ESI EDI (2 * src.ResX)  |  EAX = SRC_HANDLER   EBX = DEST_HANDLER     ecx = ResY_	EDX = (2 * dst.ResX)	 ESI = 0 EDI = (dest ptr(x,y))

; THIS is scan line copy loop	
	.PutBitmap_loopY:
		push eax
		push ebx
		push ecx
		push edx
		push esi
		push edi
		
		mov ecx, ss:[bp - 12]
		pushad
			; copy whole line from src to dst
		moveExtMem  ax, esi, bx, edi, ecx 
		popad
		
		pop edi
		pop esi
		pop edx
		pop ecx
		pop ebx
		pop eax
		
		; jump to next line
		add esi, ss:[bp - 12]
		add edi, edx
		
	loop .PutBitmap_loopY
	
	; cleaning
	pop edi ;  STACK  : BP ESI EDI   | edi =  (2 * src.ResX)
	
	pop edi
	pop esi
	
	pop bp
ret


; Load bmp file  
; ds:dx = adres nazwy  
; ds:si <- image handler              
LoadBmp:
	mov al,02d     ; al<-02h open file to read
	call OpenFile  
	mov handlef,ax  ;  store file handler 
	
	mov bx,ax      ;  copy file handler to  bx
	mov cx,54d     ; read first 54 bytes (bmp header)
	lea dx, header   ; and store it into structure
	call ReadFile  ; read

	; SI - is pointer to TIMAGE (bitmap handler)
	; allocate data for image of size header.ResX x header.ResY 
	mov edi, [header.ResX]	
	mov EAX, [header.ResY]
	call NewImg
	
	; memory allocated, now it is time to load data
	
	mov ecx,[header.ResY]
        mov eax,[header.resX]
	
	; BMP store image upside down so we decrement pointer ecx
bmpYLoop:
	dec ecx			

	push ecx			; save y pos			;  STACK : Y_POS
	push eax			; save resX	 			;  STACK : Y_POS RES_X
	mov bx, handlef
	mov ecx, eax 		; cx = resX
	shl ecx, 1			;
	add ecx, eax        ; 3 * RESX
	xor edx, edx
	lea dx, hline       ;  BX - file handle  cx - byte to read
						; we read image line by line, each line in 24 bit image has 3 bytes per pixel (3 * RESX)
						
    call ReadFile
	

	pop ecx				; get X dim 			;  STACK : Y_POS      pop : RES_X -> ecx
	pop ebx				; get Y pos				;  STACK :      	  pop : RES_X -> ecx  Y_POS->EBX
	
	push ebx			; save Y pos			;  STACK : Y_POS
	push ecx			; save X dim			;  STACK : Y_POS   RES_X 
	
	
	push esi			;  STACK : Y_POS   RES_X  IMAGE_HANDLE 
	push edi			;  STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI
	
	push esi			;  STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE
	push edi			;  STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI
	
	push ebx			;  STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI Y_POS
	push ecx		 	;  STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI Y_POS RES_X
	
	lea si, hline
	lea di, hline	

	; now we have to convert RGB_888 to RGB_565 
	
	; convertion loop RGB_888 to RGB_565 	
	.bmpXLoop:
		xor ax, ax			
		mov al, [si]
		shr al, 3
		
		xor bx, bx
		mov bl, [si + 1]
		shr bl, 2
		shl bx, 5		
		add ax, bx
		
		xor bx, bx
		mov bl, [si + 2]
		shr bl, 3
		shl bx, 11		
		add ax, bx
		mov [di], ax
	
		;mov eax, [si]  ; read RGBR'
		;xor ebx, ebx
		;mov ebx, eax	;		
		;shr ebx, 27
		;and ebx, 01Fh   ; ebx - R5		    27 -> 0
				
		;mov edx, eax    ;  		
		;shr edx, 8		
		;and edx, 03Fh	; edx - G6
		;shl edx,  5
		;add ebx, edx	; ebx = G6R5
		;
		;mov edx, eax
		;shr edx, 18	
		;and edx, 01fh
		;shl edx, 11	
		;add ebx, edx				
		
		;mov [di], bx
		
		add edi, 2
		add esi, 3
	loop .bmpXLoop

	; at this point we do have converted scan line to RGB 565
	; we have to copy it to destination image

	; TODO send xms
	xor eax, eax
	mov ax, cs
	shl eax, 16	
	lea ax, hline
	

			
	pop ecx			;   STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI Y_POS   				  |   pop : ECX -> resX	
	push eax		;   STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI Y_POS  (long ptr hline) |   ECX -> resX
	
	
	
	mov ebx, ecx	
	add ebx, ecx
	mov eax, ebx
	pop esi			;   STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI Y_POS   |   ECX = resX    EBX = 2 * resX	 EAX = RES_X * 2  ESI = (long ptr hline)
	
	pop edi		;   STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI      			|   ECX = resX    EBX = 2 * resX	 EAX = RES_X * 2  ESI = (long ptr hline) EDI = Y_POS	
	mul edi		; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI  RES_X * 2 	|   EDX:EAX = 2 * RES_X * Y_POS   ECX = resX    EBX = 2 * resX   ESI = (long ptr hline) EDI = Y_POS	
	
	mov ecx, eax  ; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI   RES_X * 2  |   EDX:EAX = 2 * RES_X * Y_POS   ECX = 2 * RES_X * Y_POS    EBX = 2 * resX   ESI = (long ptr hline) EDI = Y_POS	
	mov eax, esi  ; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE EDI   RES_X * 2   |   EAX = (long ptr hline) ECX = 2 * RES_X * Y_POS    EBX = 2 * resX   ESI = (long ptr hline) EDI = Y_POS	
	
	pop edi 	  ; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI IMAGE_HANDLE       |   EAX = (long ptr hline) ECX = 2 * RES_X * Y_POS    EBX = 2 * resX   ESI = (long ptr hline) EDI = restore edi	
	pop esi		  ; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  EDI 					|   EAX = (long ptr hline) ECX = 2 * RES_X * Y_POS    EBX = 2 * resX   ESI = IMAGE_HANDLE EDI = restore edi	
	
	pushad
	mov edi, eax		
	moveExtMem 0, edi, [SI+HANDLE_], ecx, ebx
	 cmp al,1  ; check copy error
   je ooookuk
     mov ah,4ch
    int 21h
  ooookuk:
	popad
	
	pop edi		 ; 	STACK : Y_POS   RES_X  IMAGE_HANDLE  				|    EDI = restore edi
	pop esi		 ; 	STACK : Y_POS   RES_X  				  				|    EDI = restore esi
	
	pop eax		; 	STACK : Y_POS   	  				  				|    ESI = IMAGE_HANDLER EDI = restore esi  EAX = RES_X
	pop ecx     ; 	STACK : 		   	  				  				|    ESI = IMAGE_HANDLER EDI = restore esi  EAX = RES_X   ECX = Y_POS
		
	
	push edi
	push esi
	push eax
	push ecx	; STACK :  EDI   IMAGE_HANDLE  RES_X  Y_POS

	; each bmp scan line is aligned to multiplication of 4 so we might need to skip few bytes
	; if (3*RESX) mod 4 is equal 0 then each scan line is already aligned
		
	; READ align START
	xor ebx, ebx
	mov bx, handlef
	mov ecx, eax 		; ecx = resX
	shl ecx, 1			;
	add ecx, eax        ; 3 * RESX
	and ecx, 3h			; ((3 * RES_X) mod 4)
	cmp ecx, 0
	je .bmpAligned
	mov eax, 4
	sub eax, ecx
	mov ecx, eax		; ecx = 4 - ((3 * RES_X) mod 4)
	lea edx, hline       ; WA : BX - handle  cx -ilosc w bajtach 
						; write to hline
    call ReadFile	
	.bmpAligned:
	; READ ALIGN END
	
	pop ecx
	pop eax
	pop esi
	pop edi
	
	cmp ecx, 0	; check if we read all lines
	jne bmpYLoop
	
	
ret 

; helper for file handler
handlef DW ?

; BMP file header strucutre
HeaderDef  struc             ; 54 bytes
  Imgtype  DW ?
  FileSize DD ?
  res1     DW ?  ;=0
  res2     DW ?  ;=0
  offbit   DD ?
  SizeNinf DD ?
  ResX     DD ?
  ResY     DD ?
  NRPlanes DW ?  ;=1
  bpp      DW ?
compresion DD ?
  ISize    DD ?
  DpiX     DD ?
  DPiY     DD ?
  ColUse   DD ?
  NrCol    DD ?
 ends
 
header    HeaderDef <?> 
 
; Back to text mode, ill VESA graphic and free SCREEN_BUFFER
StartTxt:
;jmp @ok_v2
 mov ax,0003h
 int 10h
 @ok_v2:
 lea si, SCREEN_BUFFER
 call freeImg
ret 

; helpers buffer 
hline DB 2000 dup(00h)

hline2 DB 2000 dup(00h)

linia DB 1280 dup(0ffh)


; Wait for vertical sync 
vsync:
  push ax
  push dx
  mov dx, 03dah
  @wait:
  in al, dx
  and al, 08h
  jz @wait
  pop dx
  pop ax
ret
