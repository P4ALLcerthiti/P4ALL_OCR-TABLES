### P4A_OCR-TABLES

A module that exports scanned documents (image or .pdf files) to .html, recognizing tabular structures. A description of the recognition algorithm can be found here: [Extraction of Tabular Data from Document Images](https://doi.org/10.1145/3058555.3058581)

The repository contains 5 directories

1) [ocr_tables](https://github.com/P4ALLcerthiti/P4ALL_OCR-TABLES/tree/master/ocr_tables) : This includes the source code that generates the OCR_TABLES.dll

2) [App](https://github.com/P4ALLcerthiti/P4ALL_OCR-TABLES/tree/master/App) : This includes the source code for a sample Qt-based app to test the module

3) [tessdata](https://github.com/P4ALLcerthiti/P4ALL_OCR-TABLES/tree/master/tessdata) : This includes the traindata necessary for the OCR engine. The tessdata folder must be in the same directory as the executable

4) [test files](https://github.com/P4ALLcerthiti/P4ALL_OCR-TABLES/tree/master/test%20files) : This includes some sample files to test the module

5) [WebService](https://github.com/P4ALLcerthiti/P4ALL_OCR-TABLES/tree/master/WebService) : This includes the .php files for a sample webservice implementation


### Dependencies

The following libraries were used to build and test the module. Older subversions may also be compatible

[OpenCV 2.4.9] (http://opencv.org/) : Used by the ocr_tables module for image processing  
opencv_core249.lib, opencv_highgui249.lib, opencv_imgproc249.lib

[MuPDF 1.7] (http://mupdf.com/) : Used by the ocr_tables module for pdf processing  
libmupdf.lib, libthirdparty.lib  
Also set /NODEFAULTLIB:"libcmt.lib"

[Tesseract-OCR 3.0.4] (https://github.com/tesseract-ocr/tesseract) : Used by the ocr_tables module for OCR  
libtesseract304.lib

[Leptonica 1.7.1] (http://www.leptonica.com/) : Used by Tesseract-OCR for image processing, and for Document Image Analysis  
liblept171.lib

[Qt 5.1.0] (http://www.qt.io/download-open-source/) : Used to build the sample App  
qtmain.lib, Qt5Core.lib, Qt5Gui.lib, Qt5Widgets.lib.   
Also don't forget to copy Qt's "platforms" directory in the same folder as the executable

### DLL Usage

```
//include header file
#include "ocr_tabs.h"
//create object  
ocr_tabs* tab = new ocr_tabs();
//process pdf file
tab->pdf2html(filename.toStdString();
//process image file
tab->img2html(filename.toStdString();
```

### Standalone App usage

Load an image or pdf file using the "LOAD" button. After the processing is finished an html file is create at "filename" + .html which can be opened using the "SHOW" button

### WebService App usage

Define "_SERVICE" to build the App in console mode, without GUI. The application takes as an argument a filename (image or .pdf) and produces the .html file. In this mode it can be called from a webservice

### Current limitations (to be updated in next version)

The module works best for single column horizontal text, for both single and multi-page documents.
Support for multi-column text and in-text images has been added in the updated version, however text/image segmentation may sometimes fail.
Non-manhattan document layouts and vertical text are not supported yet

### Citation
Please cite the following paper in your publications if it helps your research:

    @inproceedings{vasileiadis2017extraction,
      author = {Vasileiadis, Manolis and Kaklanis, Nikolaos and Votis, Konstantinos and Tzovaras, Dimitrios},
      booktitle = {Proceedings of the 14th Web for All Conference on The Future of Accessible Work},
      title = {Extraction of Tabular Data from Document Images},
      pages={24},
      organization={ACM},
      year = {2017}
    }  

### Funding Acknowledgement

The research leading to these results has received funding from the European
Union's Seventh Framework Programme (FP7) under grant agreement No.610510
