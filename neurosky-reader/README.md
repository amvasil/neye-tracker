This is a C++ Qt client for reading raw EEG data from neurosky tool:
http://neuromatix.ru/tovary/neyro-garnitury/neurosky-mindwave-mobile-starter-kit-neyro-garnitura.html

To start working, plug a Bluetooth 2.0 usb adapter into PC, deploy an application with required Qt libraries and indulge yourself.

The NeuroSky device must be connected and paired before operation, this is done as usual in Windows. After pairing bluetooth connection is visible as two virtual COM-ports.

To compile, you'll need FFTW binaries for Windows:
http://www.fftw.org/install/windows.html

and Qwt-6.1.3: http://qwt.sourceforge.net/qwtinstall.html

Originally build on Windows x64, Qt 5.5.1 MinGW
