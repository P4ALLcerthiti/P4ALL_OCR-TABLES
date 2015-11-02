P4A_OCR-TABLES

A module that exports scanned documents (image or .pdf files) to .html, recognizing tabular structures. A description of the recognition algorithm can be found here: [http]

The repository contains 4 directories

1)[ocr_tables]() : This includes the source code that generates the OCR_TABLES.dll

2)[App]() : This includes the source code for a sample Qt-based app to test the module

3)[tessdata]() : This includes the traindata necessary for the OCR engine

4)[test files]() : This includes some sample files to test the module


### Dependencies

The following libraries were used to build and test the module. Older subversions may also be compatible

[OpenCV 2.4.9] (http://opencv.org/) : Used by the ocr_tables module for image processing
opencv_highgui249.lib
opencv_imgproc249.lib
opencv_core249.lib

[MuPDF 1.7] (http://mupdf.com/) : Used by the ocr_tables module for pdf processing
libmupdf.lib
libthirdparty.lib

[Tesseract-OCR 3.0.4] (https://github.com/tesseract-ocr/tesseract) : Used by the ocr_tables module for OCR
libtesseract304.lib

[Leptonica 1.7.1] (http://www.leptonica.com/) : Used by Tesseract-OCR for image processing
liblept171.lib

[Qt 5.1.0] (http://www.qt.io/download-open-source/) : Used to build the sample App
qtmain.lib
Qt5Core.lib
Qt5Gui.lib
Qt5Widgets.lib


### App usage

Load an image or pdf file using the "LOAD" button. After the processing is finished an html file is create at "filename" + .html which can be opened using the "SHOW" button

### Current limitations (to be updated in next version)

The module currently supports single column horizontal text.
The input data can be single image or pdf (single and multi page) files. In cases of multi-page files, the module checks for header-footers and removes them.
Page segmentation for multiple text columns, non-manhattan layouts and images is not yet implemented

### Funding Acknowledgement

The research leading to these results has received funding from the European
Union's Seventh Framework Programme (FP7) under grant agreement No.610510# P4ALL_OCR-TABLES 
