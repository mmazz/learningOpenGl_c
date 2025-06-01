# OpenGL with C

This proyect is for personal learning.
The goal is to learn:
  - C
  - Memory managment for huge amount of individual elements
  - Rendering
  - More

## Requirements

For arch linux:
```
sudo pacman -Syu base-devel glfw mesa glu glew
git clone https://aur.archlinux.org/yay-bin.git
cd yay-bin
makepkg -si
yay -S cglm
```

## Resources

https://www.glfw.org/docs/3.3/quick.html

https://learnopengl.com/Advanced-OpenGL/Instancing

For nvim compile data, use bear:
```
sudo pacman -S bear
bear -- make
```


## Font

I'm using from https://www.checkmyworking.com/cm-web-fonts/

Help: [Resource](https://dev.to/shreyaspranav/how-to-render-truetype-fonts-in-opengl-using-stbtruetypeh-1p5k)


## Learnings

### Compiler

If I put in the gcc -Iinclude, basically its saying were to find the files I'm usuing.
So when I do in any .c the inclusion of the headers, it dont need to use relative paths.
Only do #include header.h.


### Render:

- See:  Programmable Vertex Pulling

### OpenGL

La idea es primero generar los VAO, que tienen la forma de leer los atributos.
Luego genero los VBO y luego cada vez que quiero cambiar de objeto bindeo el VAO correspondiente.

- VBO Vertex Buffer Objects: Donde estan las posiciones, colores y demas datos de cada vertice (punto)
- VAO Vertex Array Object: La informacion de como leo cada elemento del VBO
- EBO Element Buffer Objects: Guardo indices sobre que elementos del VBO quiero dibujar, permitiendo dibujar multiples triangulos con solo 4 puntos.
Creo que se deja de usar glDrawArrays, para usar glDrawElements.
