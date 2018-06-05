; http://www.techhelpmanual.com/944-xms_functions.html
; http://www.phatcode.net/res/219/files/xms30.txt

; far pointer to XMS function
; to get address call initializeXMS function, after successfull checkXMSinstalled
address dd ?

; structure for XMS driver to send data from
; SourceHandler, DestHandler - handler retrived by allocate mem
; handlers are id in XMS for allocated memory block
; SourceOffset, DestOffset - Source/Dest byte inside SourceHandler/DestHandler 
; LengthXMS - byte amount to copy from  SourceHandler:SourceOffset to DestHandler:DestOffset
TMemMove struc
 LengthXMS    DD 0
 SourceHandle DW 0 
 SourceOffset DD 0
 DestHandle   DW 0
 DestOffset   DD 0
ENDS

; single thread applicatnio - one structure
ExtMemMove TMemMove <?>

; This is macro to simply set TMemMove structure
; it will send LengthP bytes from source to destination
MoveExtMem macro SurH,SurO,DestH,DestO, LengthP
 mov ax,SurH
 mov [ExtMemMove.SourceHandle],ax
 mov ax,DestH
 mov [ExtMemMove.DestHandle],ax
 mov eax,SurO
 mov [ExtMemMove.SourceOffset],eax
 mov eax,DestO
 mov [ExtMemMove.DestOffset],eax
 mov eax,LengthP
 mov [ExtMemMove.LengthXMS],eax

; this is method to execute inside XMS handler
; works like interrupts, but insted of using "int xx" we do call to XMS driver  
 mov AX, 0B00h       
 lea si, ExtMemMove 
 call [Address]    
endm

; Check if we have XMS driver 
; execute driver is registered, and working method 4300h int 2fh should return 0x80 value
; this method return ax = 1 if ok, and ax = 0 if driver does not exists
checkXMSinstaled:
  mov ax, 4300h
  int 2fh
  cmp al, 80h
  je .ok
   mov ax, 0
   jmp .exitCheckXMSinstaled
  .ok:
   mov ax, 1
  .exitCheckXMSinstaled:
ret

; Call this method to get far pointer to XMS
; After execute int 2fh func 4310h should return pointer in es:bx, save it in "address"
initializeXMS:
 push es
 push di
 push bx
  mov ax, 4310h
  int 2fh
  lea di, address
  mov [di],bx
  mov [di+2],es
 pop bx 
 pop di
 pop es
ret

; Function return total RAM available for allocation in AX
totalFreeMem:
   mov ax,0800h
   call [address]
   mov ax, dx  
ret

; function return largest free RAM block for allocation
largestFreeMemBlock:
   mov ax,0800h
   call [address]   
ret


; eax - allocation size in bytes
; di - handler destination address
; result : ax == 01 Success, XMS handler in edi
; 		   ax == 00 Fail,
;		     BL = 80h if the function is not implemented
;            BL = 81h if a VDISK device is detected
;            BL = A0h if all available extended memory is allocated
;            BL = A1h if all available extended memory handles
;				are in use 
allocateMem:
	xor dx, dx
	mov [di], dx ; handler to 0
	mov edx, eax
	and edx, 03ffh
	shr eax, 10	
	cmp edx, 0
	je .perfectSize
		inc eax
	.perfectSize:
	mov edx, eax
	mov ax, 0900h 
	call [Address]	
	cmp ax, 0
	je .errorAlloc	
		mov [di], dx
	.errorAlloc:
 ret

 ; dx handler received from allocateMem
 ; result al == 01 ok
freeMem: 
   mov AX, 0A00h      
   call [Address]
ret   

