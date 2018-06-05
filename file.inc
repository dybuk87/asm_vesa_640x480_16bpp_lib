; Create new file
; DS:DX - pointer to file name
NewFile proc  
	xor cx,cx
 	mov ah,03ch
 	int 21h
	ret
endp

; Open existing file
; DS:DX - pointer to file name
; AL - access type (0 - Read only, 1 - Write only, 2 Read and Write
; AX - return file handler, this handler is used by read, write end close methods
OpenFile proc 
 mov ah,03dh
 int 21h
 jnc open_file_success
  mov ax,0003h
  int 10h 
  mov ah,09
  mov dx, offset errorOpen
  int 21h
  mov ah,4ch
  int 21h
open_file_success:
 ret
endp


; Read bytes from file
; BX - file handler from OpenFile
; CX - bytes to read
; DX:DX - destination array
ReadFile proc  
 push eax
 mov ah, 03fh
 int 21h
 jnc read_file_success
  mov ax,0003h
  int 10h 
  mov ah,09
  mov dx, offset errorRead
  int 21h
  mov ah,4ch
  int 21h
read_file_success:
 pop eax
  ret
endp


; Write bytes to file
; BX - file handler from OpenFile
; CX - how many bytes to save
; DS:DX - pointer to array to save 
SaveFile proc 
 mov ah,040h
 int 21h
 ret
endp

; Close file 
; BX - file handler from OpenFile
CloseFile proc 
 mov ah,03eh
 int 21h
 ret
endp

errorOpen DB "Open file error. $"
errorRead DB "Read file error. $"
errorSave DB "Write file error. $" 
