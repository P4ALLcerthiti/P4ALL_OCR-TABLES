#pragma once
#include "dll_config.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <ctime>



 #define TESSDLL_IMPORTS

#include "tesseract/baseapi.h"

using namespace cv;
using namespace std;

class ocr_tabs
{
	public:
			OCRTABS_API ocr_tabs ();
			OCRTABS_API ~ocr_tabs ();
			void SetImage(Mat img);
			void RemoveGridLines();
			void OCR_Recognize();
			void BoxesAndWords();
			void TextBoundaries();
			void TextLines();
			void HeadersFooters();
			void LineSegments();
			void LineTypes();
			void TableAreas();
			void TableRows();
			void TableColumns();
			void TableMultiRows();
			void ColumnSize();
			void FinalizeGrid();
			void WriteHTML(std::string& filename);
			void PrepareMulti1();
			void PrepareMulti2();
			Mat ImgSeg(Mat img);
			Mat getInitial (){return initial;}

			void DrawBoxes();
			void DrawLines();
			void DrawSegments();
			void DrawAreas();
			void DrawRows();
			void DrawColsPartial();
			void DrawCols();
			void DrawGrid();
			void DrawGridlessImage();
			//void DrawFootHead();

			void ResetImage();
			Mat ImagePreproccesing(Mat img);
			
			bool fail_condition();
			bool OCRTABS_API pdf2html (const std::string& filename);
			bool OCRTABS_API img2html (const std::string& filename);
			bool parsePDF(const std::string& filename, std::vector<cv::Mat>& imageList);
			void resetAll();

private:
			Mat test,initial;
			tesseract::TessBaseAPI  tess;
			clock_t start;
			double duration;
			vector<char*> words;
			vector<vector<int>> boxes, Lines, table_area, table_Rows;
			vector<vector<vector<int>>> multi_Rows;
			vector<int*> Line_dims;
			vector<vector<vector<int>>> Lines_segments;
			vector<vector<vector<vector<int>>>> table_Columns;
			vector<vector<int*>> col_dims, row_dims;
			vector<vector<vector<int>>> tmp_col;
			vector<float> confs;
			vector<bool> bold;
			vector<bool> dict;
			vector<bool> italic;
			vector<bool> underscore;
			vector<int> font_size;
			int page_left, page_right, page_top, page_bottom;
			int* Lines_type;
			vector<vector<char*>> words_;
			vector<vector<vector<int>>> Lines_;
			vector<vector<vector<int>>> boxes_;
			vector<vector<float>> confs_;
			vector<vector<int>> font_size_;
			vector<vector<bool>> bold_;
			vector<vector<bool>> dict_;
			vector<vector<bool>> italic_;
			vector<vector<bool>> underscore_;
			vector<int> page_height,page_width;
			bool fail;
};
