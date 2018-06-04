NewFile proc  ; ds:dx = adres nazwy
 xor cx,cx
 mov ah,03ch
 int 21h
 ret
endp

OpenFile proc ;WA : ds:dx = adres nazwy al= tryb dostepu(0=R, 1=W ,2=RW)
              ;WY : ax - dojscie
 mov ah,03dh
 int 21h
 jnc dalejOpen
  mov ax,0003h
  int 10h 
  mov ah,09
  mov dx, offset errorOpen
  int 21h
  mov ah,4ch
  int 21h
  ret
 DalejOpen:
 ret
endp

ReadFile proc ;WA : BX - handle  cx -ilosc w bajtach 
              ;WY : ds:dx -> wyjscie danych
 push eax
 mov ah, 03fh
 int 21h
 jnc DalejRead
  mov ax,0003h
  int 10h 
  mov ah,09
  mov dx, offset errorRead
  int 21h
  mov ah,4ch
  int 21h
  ret  
  DalejRead:
 pop eax
  ret
endp

SaveFile proc ;WA : BX - handle  cx - ilosc bajtow  Ds:dx ->poczatek danych
 mov ah,040h
 int 21h
 ret
endp

CloseFile proc ;WA : BX -Handle
 mov ah,03eh
 int 21h
 ret
endp

errorOpen DB "Blad przy otwieraniu pliku. $"
errorRead DB "Blad przy odczycie z pliku. $"
errorSave DB "Blad przy zapisie do pliku. $" 