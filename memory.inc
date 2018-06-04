; http://www.techhelpmanual.com/944-xms_functions.html
; http://www.phatcode.net/res/219/files/xms30.txt

address dd ?

TMemMove struc
 LengthXMS    DD 0
 SourceHandle DW 0 
 SourceOffset DD 0
 DestHandle   DW 0
 DestOffset   DD 0
ENDS

ExtMemMove TMemMove <?>

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

 mov AX, 0B00h       
 lea si, ExtMemMove 
 call [Address]    
endm


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


totalFreeMem:
   mov ax,0800h
   call [address]
   mov ax, dx  
ret

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

 ; dx handler
 ; result al == 01 ok
freeMem: 
   mov AX, 0A00h      
   call [Address]
ret   

