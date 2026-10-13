# AeroRAM

Aplicación portátil de prueba para Windows x64, creada para Cristian Skylar.
Combina monitor de RAM, limpieza manual y un panel de especificaciones del
sistema. Versión 0.4.3.

## Abrir

1. Cierra la versión anterior desde su icono junto al reloj > **Salir**.
2. Descomprime este ZIP en una carpeta nueva.
3. Abre **AeroRAM.exe** y acepta la solicitud de administrador de Windows.

No requiere instalar Python, Node ni .NET. El logo y el icono están integrados
en el ejecutable; no depende de las imágenes de referencia para abrir.

## Licencia y publicación en GitHub

AeroRAM incluye una licencia personalizada de uso y redistribución sin
modificaciones, a nombre de Cristian Skylar. Permite usar y compartir copias
originales conservando la licencia, la autoría y los avisos de terceros.
No concede permiso para modificar los elementos propios del programa ni
crear versiones derivadas sin autorización escrita. El texto completo está
en LICENSE; los componentes de terceros conservan sus propias licencias.

Al crear el repositorio, selecciona **No license** y después sube el archivo
LICENSE de este paquete a la raíz. No añadas una licencia MIT, GPL, Apache o
similar, porque concedería permisos distintos de los de este paquete.
Si el repositorio es público, los términos de GitHub permiten a sus usuarios
verlo y hacer forks dentro de la plataforma. Una licencia no es una barrera
técnica contra la copia o la alteración de archivos.

Este paquete cambia únicamente la licencia y su documentación; el ejecutable
y el código del programa siguen siendo los de la versión 0.4.3.
El texto de licencia es una propuesta personalizada y no ha sido revisado
por un profesional jurídico. No revoca permisos de copias previamente
distribuidas válidamente bajo otra licencia.

## Cambios de 0.4.3

- El fondo del panel es opaco en ambos temas. Las ventanas y el escritorio que queden detrás ya no se ven a través de AeroRAM.
- Se conservan los degradados, reflejos, bordes de vidrio, profundidad de las tarjetas y animaciones. Las tarjetas se componen sobre el fondo opaco del propio programa.
- Las cuatro esquinas conservan su recorte suavizado y las funciones de memoria, diagnóstico y créditos permanecen iguales.
- El código sigue sin comentarios.

## Cambios de 0.4.2

- La descripción de Sobre el programa dice «para Windows».
- La compilación sigue siendo x64. No contiene una comprobación que impida abrirla en Windows 11; se espera compatibilidad con Windows 10 y Windows 11 en equipos x64, pero esta versión no se ha ejecutado aquí en ninguno de los dos sistemas.
- Cambiar la descripción no modifica las funciones de monitor, limpieza o diagnóstico. La compatibilidad completa, en particular los métodos internos de limpieza, requiere pruebas reales en cada versión de Windows.

## Cambios de 0.4.1

- Se retira el desenfoque nativo rectangular que invadía las esquinas transparentes. Esta versión conserva el vidrio translúcido, los reflejos y las animaciones; ya no desenfoca el escritorio detrás de la ventana.
- Las cuatro esquinas comparten una máscara de transparencia con bordes suavizados, aplicada antes de presentar cada fotograma. Los puntos transparentes quedan fuera de la zona de clic.
- Al cambiar de escala DPI se reconstruyen la superficie y la máscara antes de volver a dibujar, para evitar mezclar tamaños anteriores y nuevos.
- Se verifican nueve escalas entre 50 % y 300 %, las esquinas inferiores y superiores, el alfa de ambos temas y los límites del búfer.
- Todo el código del programa y de las pruebas sigue sin comentarios.

## Créditos de 0.4

- Botón Créditos entre Sistema y Ajustes, y acceso desde el menú del icono junto al reloj.
- Vista de créditos con logo, versión, descripción y **Desarrollador: Cristian Skylar**.
- Iconos de Instagram, X, LinkedIn y sitio web; cada tarjeta abre la red correspondiente en el navegador predeterminado.
- Las fuentes C++ y las pruebas ya no contienen comentarios. La lógica de memoria y del diagnóstico se conserva, comprobada comparando el código sin comentarios.

## Créditos y redes

Los iconos usan geometrías SVG convertidas a trazados vectoriales nativos para
conservar la nitidez al escalar. Los SVG originales se incluyen en
resources/social y se integran como recursos del ejecutable.

- Instagram: https://www.instagram.com/CristianSkylarr/
- X: https://x.com/CristianSkylarr
- LinkedIn: https://www.linkedin.com/in/cristianskylar/
- Sitio web: https://cristianskylar.com

## Cambios de 0.3

- Nuevo nombre AeroRAM y logo de memoria verde con alas blancas, en la ventana,
  el ejecutable y el área de notificación.
- Se elimina el margen transparente de 16 px del tamaño real de la ventana.
  El fondo se dibuja con origen ajustado y ya no se extiende una hoja de
  cristal a toda la superficie mediante márgenes DWM negativos.
- Se mantiene el recorte redondeado a escala DPI y se deshabilita el marco
  no cliente de DWM. El aspecto final del compositor requiere prueba en Windows.
- La cifra usada / total dentro del círculo sube 10 px.
- Botón **Sistema** abajo a la izquierda. Los resultados de limpieza pasan a
  la línea situada sobre el botón principal, y conservan sus detalles al pulsarlos.
- Nueva vista **Tu equipo**, con ocho apartados, lectura progresiva, rueda y
  barra de desplazamiento, actualización, copia del informe y acceso a dxdiag.

## Tu equipo

La consulta se inicia al abrir Sistema; se ejecuta en un hilo independiente.
Los resultados se conservan durante la sesión y solo se releen al pulsar
Actualizar. La ventana muestra datos locales proporcionados por Windows.

| Apartado | Datos consultados |
| --- | --- |
| Sistema | Fabricante y modelo, nombre del equipo, Windows y compilación, placa base y revisión, BIOS y fecha, SMBIOS, UUID, modo de arranque BIOS/UEFI. |
| CPU | Modelo, fabricante, socket, núcleos e hilos, frecuencias informadas, cachés, ancho de direcciones e identificador. |
| RAM | Módulos, capacidad, fabricante, referencia y serie, ranura y banco, DDR, DIMM/SO-DIMM, velocidad nominal y configurada, voltaje y ancho de datos. |
| Gráficos | Adaptadores, fabricantes, chips, controladores y fechas, resolución y frecuencia, VRAM dedicada y memoria compartida, PCI, nivel disponible en Direct3D 11, datos DirectX de DxDiag. |
| Discos | Modelos, fabricante, firmware, series, capacidad, particiones, volúmenes y espacio libre; SSD/HDD, bus NVMe/SATA y sectores cuando el proveedor Storage los reporta. |
| Red | Adaptadores físicos, fabricante, estado, enlace, MAC, direcciones IP, puerta de enlace, DHCP y DNS. |
| Audio | Dispositivos, fabricante, estado e identificadores. |
| Dispositivos | Teclado, ratón, HID y monitores con fabricante, clase, estado e identificadores PnP. |

**Copiar informe** copia todos los apartados y los valores completos, sin el
recorte visual de una fila. Está disponible una vez terminada la lectura.
**Abrir dxdiag** abre la herramienta incluida en Windows, para consultar sus
pruebas y detalles adicionales. El proveedor DxDiag interno se inicializa sin
comprobaciones WHQL en línea.

Las propiedades ausentes, desconocidas o rellenadas con marcadores OEM como
"To Be Filled By O.E.M." aparecen como **No reportado**. No se deduce la marca
ni el modelo a partir de una imagen o de datos de otra máquina.
Las consultas WMI tienen límites de espera y enumeración; un proveedor que
falle puede dejar un apartado sin datos. La vista es un inventario diagnóstico,
no una prueba de rendimiento ni una sustitución completa de dxdiag.
La VRAM proviene de DXGI con capacidades de 64 bits, para evitar el límite de
la propiedad AdapterRAM de WMI. RAM/VRAM se expresan en GiB en este panel y
los discos en GB decimales. La versión de DirectX instalada y el nivel que
acepta Direct3D 11 son datos distintos.

## Monitor y limpieza

- RAM usada, disponible y total reales mediante GlobalMemoryStatusEx.
- Transiciones animadas e histórico de hasta 60 muestras.
- Limpieza manual en un hilo separado, con resultados antes/después.
- Métodos independientes: reducción de working sets y purga de standby de
  baja prioridad con NtSetSystemInformation, clase 80, comandos 2 y 5.
- Privilegio SeProfileSingleProcessPrivilege temporal, restauración y
  comprobación de errores y resultados parciales.
- Temas claro/oscuro, animaciones opcionales y minimización al área de notificación.
- No incluye telemetría, autoarranque ni limpieza automática.

El resultado mide el cambio de RAM disponible y puede verse afectado por
otras aplicaciones. Una limpieza puede producir cero aumento; no garantiza
más velocidad. Las aplicaciones pueden recargar páginas al volver a usarlas.

## Controles y ajustes

Tab / Mayús+Tab y Enter permiten recorrer y activar controles. Escape vuelve
al monitor o esconde la ventana. En Sistema, rueda o arrastre de la barra
permiten desplazarse; Inicio/Fin y RePág/AvPág también funcionan. Ctrl+C copia
el informe completo y F5 actualiza. Las flechas izquierda/derecha cambian de
apartado cuando su pestaña tiene el foco.

Al cerrar, se oculta junto al reloj si "Minimizar al cerrar" está activo.
Para salir completamente: clic derecho en ese icono > **Salir**.
Los ajustes se guardan en LocalAppData\AeroRAM\preferences.ini. Se importan
los ajustes anteriores de SkylarMemory si AeroRAM todavía no tiene un archivo.

## Validación y límites

Compilación GCC/MinGW-w64 C++17 con -Wall -Wextra -Wpedantic -Werror.
Se revisan PE x64 gráfico, recursos de versión e iconos, manifiesto de
administrador/DPI e importaciones; las DLL enlazadas forman parte de Windows.
Las pruebas portátiles del informe comprueban marcadores OEM, capacidades de
64 bits y desbordamientos, unidades, fechas y copia completa de todos los
apartados, incluidos identificadores largos. Las pruebas de lógica de memoria
cubren cambios negativos, muestras inválidas, resultados parciales e histórico.

Las capturas del usuario confirman que versiones previas abren y leen RAM
real, y muestran el defecto del blur exterior que motiva el cambio.
El usuario expresó satisfacción con la versión 0.3. La captura de la versión
anterior confirma que el blur seguía ocupando los recortes de las esquinas.
**Esta versión 0.4.3 no se ha ejecutado aquí en Windows 10 ni Windows 11.** Quedan por comprobar
el compositor, las consultas a los proveedores de tu equipo, dxdiag, la
interacción y la limpieza en el sistema real.

Las imágenes en preview son referencias de diseño con datos de ejemplo,
no capturas de Windows ni evidencia de que la consulta real funcione.
Las pruebas de la máscara pasan con AddressSanitizer y UndefinedBehaviorSanitizer,
sin detección de fugas, que este entorno no permite ejecutar. Comprueban
simetría, transparencia exterior, bordes suavizados, alfa premultiplicado,
conservación del interior, clics, filas con relleno y superficies invertidas.
El acabado de vidrio ahora se dibuja sobre un fondo opaco en la superficie de AeroRAM;
no se activa SetWindowCompositionAttribute ni el desenfoque DWM de fondo.
Esta compilación no tiene firma digital.

## Compilar

Con Visual Studio Build Tools, Desarrollo de escritorio con C++ y CMake,
ejecuta build-windows.cmd o:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

CMake también permite MinGW-w64. Los recursos del logo están en resources.
Las licencias del runtime se describen en THIRD-PARTY.txt.

Para las pruebas portátiles:

```sh
g++ -std=c++17 -O2 tests/spec_tests.cpp -o spec-tests
./spec-tests
g++ -std=c++17 -O2 tests/core_tests.cpp -o core-tests
./core-tests
g++ -std=c++17 -O2 tests/window_shape_tests.cpp -o window-shape-tests
./window-shape-tests
```

## Referencias de API

- [WMI en C++](https://learn.microsoft.com/en-us/windows/win32/wmisdk/creating-a-wmi-application-using-c-)
- [Memoria física y módulos](https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-physicalmemory)
- [Almacenamiento físico](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/msft-physicaldisk)
- [DXGI y memoria del adaptador](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-getdesc)
- [Direct3D 11 y niveles de funciones](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice)
- [GetFirmwareType](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfirmwaretype)
- [Ventanas con transparencia por píxel](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#layered-windows)
- [Márgenes de DWM](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmextendframeintoclientarea)

AeroRAM es una implementación independiente; no incluye código de Mem Reduct.
