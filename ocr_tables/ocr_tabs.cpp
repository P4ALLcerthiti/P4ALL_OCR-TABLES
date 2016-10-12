#pragma once
#include <iostream>
#include <fstream>
#include "ocr_tabs.h"
#include "imgProcessor.h"
extern "C" {
	#include <mupdf/fitz.h>
}


using namespace cv;
using namespace std;

#pragma warning( disable : 4018 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )

ocr_tabs::ocr_tabs()
{
	fail=false;
	Lines_type = NULL;
	tess.Init("..\\tessdata", "eng");
}
//////////////////////////////////////////////////////////////
ocr_tabs::~ocr_tabs()
{
	resetAll();
}
//////////////////////////////////////////////////////////////
void ocr_tabs::SetImage(Mat img)
{
	//test=ImgSeg(img);
	test=img.clone();
	initial=test.clone();
}
//////////////////////////////////////////////////////////////
void ocr_tabs::PrepareMulti1()
// Each input image is OCRed, in order to extract words and bounding boxes
// Then we check for similar lines on the top and bottom of the images in order to remove headers/footers
{
		//RemoveGridLines();
		page_height.push_back(test.size().height);
		page_width.push_back(test.size().width);
		OCR_Recognize();
		BoxesAndWords();

		for (int i=0;i<boxes.size();i++)
		{
			vector<int> tmp;
			tmp.push_back(i);
			int j;
			for (j=i+1;j<boxes.size();j++)
			{
				if (((boxes[j-1][1]<=boxes[j][1])&&(boxes[j][1]<=boxes[j-1][3]))||
					((boxes[j][1]<=boxes[j-1][1])&&(boxes[j-1][1]<=boxes[j][3])))
				{
					tmp.push_back(j);
				}
				else
				{
					break;
				}
			}
			i=j-1;
			Lines.push_back(tmp);
		}
		
		boxes_.push_back(boxes);
		words_.push_back(words);
		confs_.push_back(confs);
		Lines_.push_back(Lines);
		font_size_.push_back(font_size);
		bold_.push_back(bold);
		italic_.push_back(italic);
		underscore_.push_back(underscore);
		dict_.push_back(dict);
		
		test.release();
		confs.clear();
		boxes.clear();
		words.clear();
		Lines.clear();
		font_size.clear();
		bold.clear();
		italic.clear();
		underscore.clear();
		dict.clear();
}
//////////////////////////////////////////////////////////////	
void ocr_tabs::PrepareMulti2()
{
	for (int i=1;i<boxes_.size();i++)
	{
		int move=0;
		for (int j=0;j<i;j++)
		{
			move=move+page_height[j];
		}
		for (int j=0;j<boxes_[i].size();j++)
		{
			boxes_[i][j][1]=boxes_[i][j][1]+move;
			boxes_[i][j][3]=boxes_[i][j][3]+move;
		}
	}

	for (int i=0;i<boxes_.size();i++)
	{
		for (int j=0;j<boxes_[i].size();j++)
		{
			boxes.push_back(boxes_[i][j]);
			font_size.push_back(font_size_[i][j]);
			words.push_back(words_[i][j]);
			confs.push_back(confs_[i][j]);
			bold.push_back(bold_[i][j]);
			italic.push_back(italic_[i][j]);
			underscore.push_back(underscore_[i][j]);
			dict.push_back(dict_[i][j]);
		}
	}
	confs_.clear();
	boxes_.clear();
	words_.clear();
	Lines_.clear();
	font_size_.clear();
	bold_.clear();
	italic_.clear();
	underscore_.clear();
	dict_.clear();
	
	page_bottom=page_right=0;
	page_left=page_width[0];
	for (int i=1;i<page_width.size();i++)
	{
		page_left=std::max(page_left,page_width[i]);
	}
	page_top=0;
	for (int i=0;i<page_height.size();i++)
	{
		page_top=page_top+page_height[i];
	}
	for (int i=0;i<boxes.size();i++)
	{
		if (boxes[i][0]<=page_left){page_left=boxes[i][0];}
		if (boxes[i][1]<=page_top){page_top=boxes[i][1];}
		if (boxes[i][2]>=page_right){page_right=boxes[i][2];}
		if (boxes[i][3]>=page_bottom){page_bottom=boxes[i][3];}
	}
	cout << "\nPROCESSING OVERALL DOCUMENT\n\n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::RemoveGridLines(float ratio /*=1*/)
	// Remove grid lines, because tesseract has a problem recognizing words when the are dark, dense gridlines
{
	Mat dst;
	cout << "Remove Grid Lines...";
	start = clock();
	//threshold( test, dst, 100, 255,1 );
	threshold( test, dst, 200, 255,0 );
	//erode(dst,dst,Mat(),Point(-1,-1),2);
	uchar* data = (uchar*)dst.data;
	//cvtColor(test,test,CV_GRAY2BGR);
	for (int i=0;i<dst.cols;i++)
	{
		for (int j=0;j<dst.rows;j++)
		{
			int counter=0;
			if (data[i+j*dst.cols]==0)
			{
				while ((j<(dst.rows-1))&&(data[i+(j+1)*dst.cols]==0))
				{
					counter++;
					j++;
				}
			}
			if (counter>120*ratio)
			{
				line( test, Point(i,j-counter), Point(i, j), Scalar(255,255,255), 3);
			}
		}
	}
	for (int i=0;i<dst.rows*dst.cols;i++)
	{
		int counter=0;
		if (data[i]==0)
		{
			while ((i<(dst.rows*dst.cols-1))&&(data[i+1]==0))
			{
				counter++;
				i++;
			}
		}
		if (counter>60*ratio)
		{
			line( test, Point((i-counter)%(dst.cols),(int)(i-counter)/dst.cols), Point((i)%(dst.cols), (int)(i)/dst.cols), Scalar(255,255,255), 3);
		}
	}

	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	
	
	//cv::namedWindow("asd",CV_WINDOW_NORMAL);
	//cv::imshow("asd", test);
	//cv::waitKey(0);

	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::OCR_Recognize()
	//Tesseract recognizes all data in the image
{
	
	//resize(test,test,Size(test.size().width*2,test.size().height*2));
	//tess.Init("..\\tessdata", "eng");
	tess.SetImage((uchar*)test.data, test.size().width, test.size().height, test.channels(), test.step1());
	//tess.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!-*()");
	cout << "Recognizing...";
	start = clock();
	tess.Recognize(NULL);
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::BoxesAndWords()
	// Retrieve all the recognized words and their bounding boxes from tesseract
{
	//tess.SetPageSegMode( tesseract::PSM_AUTO_OSD);
	//tesseract::PageIterator* ri = tess.AnalyseLayout();
	tesseract::ResultIterator* ri = tess.GetIterator();

	cout << "Get bounding Boxes...";
	start = clock();
	do 
	{
		int left,top,right,bottom;
		left=0;top=0;right=0;bottom=0;
		ri->BoundingBox(tesseract::RIL_WORD, &left, &top, &right, &bottom); 
		
		// Try to discard "noise" recognized as letters
	//	if ((right-left>=10)/*&&(bottom-top>=3)*/&&(ri->Confidence(tesseract::RIL_WORD)>30))
	//	{
			bool boldword,italicword,underline,b,c,d;
			int point_size,id;
			ri->WordFontAttributes(&boldword, &italicword,&underline,&b,&c,&d,&point_size,&id);
	//		if (point_size>=2)
	//		{
				confs.push_back(ri->Confidence(tesseract::RIL_WORD));
				words.push_back(ri->GetUTF8Text(tesseract::RIL_WORD));
				font_size.push_back(point_size);
				bold.push_back(boldword);
				italic.push_back(italicword);
				underscore.push_back(underline);
		
				vector <int> tmp;
				tmp.push_back(left);
				tmp.push_back(top);
				tmp.push_back(right);
				tmp.push_back(bottom);
				boxes.push_back(tmp);
				dict.push_back(ri->WordIsFromDictionary());
	//		}
	//	}
	}while(ri->Next(tesseract::RIL_WORD));

	// remove possible figures
	// Search for word "figure". When found check the words above. if the previous 4 words are not "in dictionary" then they are
	//  part of images that have been recognised as text, and they are removed. The line with the word
	// "figure" is also removed as it is probably a caption of the figure.
	for (int i=boxes.size()-1;i>=0;i--)
	{
		string tmp=words[i];
		std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
		if (tmp.find("figure")!=std::string::npos)
		{
			for (int j=i-1;j>=3;j--)
			{
				bool image=(dict[j]&&dict[j-1]&&dict[j-2]&&dict[j-3]);
				if (image)
				{
					if (j>=(i-5))
					{
						j=0;
					}
					else
					{

					int ln_index=i;
					for (int k=i+1;k<boxes.size();k++)
					{
						if (!(((boxes[k][1]<=boxes[i][1])&&(boxes[i][1]<=boxes[k][3]))||
						((boxes[i][1]<=boxes[k][1])&&(boxes[k][1]<=boxes[i][3]))))
						{
							ln_index=k-1;
							k=boxes.size();
						}
					}
					for (int k=ln_index;k>=j+1;k--)
					{
						boxes.erase(boxes.begin()+k);
						words.erase(words.begin()+k);
						confs.erase(confs.begin()+k);
						font_size.erase(font_size.begin()+k);
						bold.erase(bold.begin()+k);
						italic.erase(italic.begin()+k);
						underscore.erase(underscore.begin()+k);
						dict.erase(dict.begin()+k);
					}
					i=j;
					j=0;
				}
				}
			}
		}
	}
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TextBoundaries()
	//Find the text boundaries of the whole image
{
	cout << "Find text boundaries...";
	start = clock();

	page_bottom=page_right=0;
	page_left=test.size().width;
	page_top=test.size().height;

	for (int i=0;i<boxes.size();i++)
	{
		if (boxes[i][0]<=page_left){page_left=boxes[i][0];}
		if (boxes[i][1]<=page_top){page_top=boxes[i][1];}
		if (boxes[i][2]>=page_right){page_right=boxes[i][2];}
		if (boxes[i][3]>=page_bottom){page_bottom=boxes[i][3];}
	}
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TextLines()
	// Create lines by assigning vertically-overlapping word boxes to the same unique line
{
	cout<< "Find lines...";
	start = clock();
	for (int i=0;i<boxes.size();i++)
	{
		vector<int> tmp;
		tmp.push_back(i);
		int j;
		for (j=i+1;j<boxes.size();j++)
		{
			if (((boxes[j-1][1]<=boxes[j][1])&&(boxes[j][1]<=boxes[j-1][3]))||
				((boxes[j][1]<=boxes[j-1][1])&&(boxes[j-1][1]<=boxes[j][3])))
			{
				tmp.push_back(j);
			}
			else
			{
				break;
			}
		}
		i=j-1;
		Lines.push_back(tmp);
	}

	// Find line dimensions (top, bottom, left, right)
	for (int i=0;i<Lines.size();i++)
	{
		int* tmp = new int[2];
		tmp[0]=page_bottom;
		tmp[1]=page_top;
		for (int j=0;j<Lines[i].size();j++)
		{
			if (boxes[Lines[i][j]][1]<=tmp[0]){tmp[0]=boxes[Lines[i][j]][1];}
			if (boxes[Lines[i][j]][3]>=tmp[1]){tmp[1]=boxes[Lines[i][j]][3];}
		}
		Line_dims.push_back(tmp);
	}

	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::HeadersFooters()
{
	int* header_limit=new int [Lines_.size()];
	int* footer_limit=new int [Lines_.size()];
	for (int i=0;i<Lines_.size();i++)
	{
		header_limit[i]=-1;
		footer_limit[i]=-1;
	}
	for (int i=0;i<Lines_.size()-1;i++)
	{
		
		//search for similar headers/footers in consecutive pages
		for (int k=0;k<std::min(Lines_[i].size(),Lines_[i+1].size());k++)
		{
			string tmp1;
			string tmp2;
			for (int j=0;j<Lines_[i][k].size();j++)
			{
				tmp1.append(words_[i][Lines_[i][k][j]]);
			}
			for (int j=0;j<Lines_[i+1][k].size();j++)
			{
				tmp2.append(words_[i+1][Lines_[i+1][k][j]]);
			}
			int identical_counter=0;
			for (int s=0;s<std::min(tmp1.length(),tmp2.length());s++)
			{
				if (tmp1[s]==tmp2[s])
				{
					identical_counter++;
				}
			}
			if ((std::max(tmp1.length(),tmp2.length())-identical_counter)<=2)
			{
				header_limit[i+1]=k;
				if ((i!=0)&&(header_limit[i]<k))
				{
					header_limit[i]=k;
				}	
			}
			else
			{
				k=std::min(Lines_[i].size(),Lines_[i+1].size());
			}
		}
		string tmp1;
		string tmp2;
		for (int j=0;j<Lines_[i][Lines_[i].size()-1].size();j++)
		{
			tmp1.append(words_[i][Lines_[i][Lines_[i].size()-1][j]]);
		}
		for (int j=0;j<Lines_[i+1][Lines_[i+1].size()-1].size();j++)
		{
			tmp2.append(words_[i+1][Lines_[i+1][Lines_[i+1].size()-1][j]]);
		}
		int identical_counter=0;
		for (int s=0;s<std::min(tmp1.length(),tmp2.length());s++)
		{
			if (tmp1[s]==tmp2[s])
			{
				identical_counter++;
			}
		}
		if ((std::max(tmp1.length(),tmp2.length())-identical_counter)<=2)
		{
			footer_limit[i]=Lines_[i].size()-1;
			footer_limit[i+1]=Lines_[i+1].size()-1;
		}
		if (i<(Lines_.size()-2))
		{
			for (int k=0;k<std::min(Lines_[i].size(),Lines_[i+2].size());k++)
			{
				tmp1.clear();
				tmp2.clear();
				for (int j=0;j<Lines_[i][k].size();j++)
				{
					tmp1.append(words_[i][Lines_[i][k][j]]);
				}
				for (int j=0;j<Lines_[i+2][k].size();j++)
				{
					tmp2.append(words_[i+2][Lines_[i+2][k][j]]);
				}
				identical_counter=0;
				for (int s=0;s<std::min(tmp1.length(),tmp2.length());s++)
				{
					if (tmp1[s]==tmp2[s])
					{
						identical_counter++;
					}
				}
				if ((std::max(tmp1.length(),tmp2.length())-identical_counter)<=2)
				{
					header_limit[i+2]=k;
					if ((i!=0)&&(header_limit[i]<k))
					{
						header_limit[i]=k;
					}		
				}
				else
				{
					k=std::min(Lines_[i].size(),Lines_[i+2].size());
				}
			}
			tmp1.clear();
			tmp2.clear();
			for (int j=0;j<Lines_[i][Lines_[i].size()-1].size();j++)
			{
				tmp1.append(words_[i][Lines_[i][Lines_[i].size()-1][j]]);
			}
			for (int j=0;j<Lines_[i+2][Lines_[i+2].size()-1].size();j++)
			{
				tmp2.append(words_[i+2][Lines_[i+2][Lines_[i+2].size()-1][j]]);
			}
			identical_counter=0;
			for (int s=0;s<std::min(tmp1.length(),tmp2.length());s++)
			{
				if (tmp1[s]==tmp2[s])
				{
					identical_counter++;
				}
			}
			if ((std::max(tmp1.length(),tmp2.length())-identical_counter)<=2)
			{
				footer_limit[i]=Lines_[i].size()-1;
				footer_limit[i+2]=Lines_[i+2].size()-1;
			}
		}
	}

	for (int i=Lines_.size()-1;i>=0;i--)
	{
		if (footer_limit[i]!=(-1))
		{
			for (int j=Lines_[i][footer_limit[i]].size()-1;j>=0;j--)
			{
				words_[i].erase(words_[i].begin()+Lines_[i][footer_limit[i]][j]);
				boxes_[i].erase(boxes_[i].begin()+Lines_[i][footer_limit[i]][j]);
				confs_[i].erase(confs_[i].begin()+Lines_[i][footer_limit[i]][j]);
				font_size_[i].erase(font_size_[i].begin()+Lines_[i][footer_limit[i]][j]);
				italic_[i].erase(italic_[i].begin()+Lines_[i][footer_limit[i]][j]);
				bold_[i].erase(bold_[i].begin()+Lines_[i][footer_limit[i]][j]);
				underscore_[i].erase(underscore_[i].begin()+Lines_[i][footer_limit[i]][j]);
				dict_[i].erase(dict_[i].begin()+Lines_[i][footer_limit[i]][j]);
			}
		}
		for (int k=header_limit[i];k>=0;k--)
		{
			for (int j=Lines_[i][k].size()-1;j>=0;j--)
			{
				words_[i].erase(words_[i].begin()+Lines_[i][k][j]);
				boxes_[i].erase(boxes_[i].begin()+Lines_[i][k][j]);
				confs_[i].erase(confs_[i].begin()+Lines_[i][k][j]);
				font_size_[i].erase(font_size_[i].begin()+Lines_[i][k][j]);
				bold_[i].erase(bold_[i].begin()+Lines_[i][k][j]);
				italic_[i].erase(italic_[i].begin()+Lines_[i][k][j]);
				underscore_[i].erase(underscore_[i].begin()+Lines_[i][k][j]);
				dict_[i].erase(dict_[i].begin()+Lines_[i][k][j]);
			}
		}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::LineSegments()
	// Create text Segments for each line. 
	// If the horizontal distance between two word boxes is smaller than a threshold,
	// they will be considered as a single text segment
{
	cout<<"Find line segments...";
	start = clock();
	
	float ratio=0.6*2; //1.2
	
	for (int i=0;i<Lines.size();i++)
	{
		if (Lines[i].size()==1)
		{
			vector<vector<int>> segments;
			vector<int> tmp;
			tmp.push_back(Lines[i][0]);
			segments.push_back(tmp);
			Lines_segments.push_back(segments);
		}
		else
		{
			float hor_thresh=(Line_dims[i][1]-Line_dims[i][0])*ratio;
			vector<vector<int>> segments;
			vector<int> tmp;
			int currPos=0;
			tmp.push_back(Lines[i][0]);
			for (int j=1;j<Lines[i].size();j++)
			{
				if (boxes[Lines[i][j]][0]-boxes[Lines[i][j-1]][2]<=hor_thresh)
				{
					tmp.push_back(Lines[i][j]);
				}
				else
				{
					segments.push_back(tmp);
					tmp.clear();
					tmp.push_back(Lines[i][j]);
				}
			}
			segments.push_back(tmp);
			Lines_segments.push_back(segments);
		}
	}
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::LineTypes()
	// Type 1 - TEXT : Lines with a single long segment (longer than half of the length of the line)
	// Type 2 - TABLE : Lines with multiple segments
	// Type 3 - UNKNOWN : Lines with a single short segment
{
	cout <<"Find line types...";
	start = clock();
	Lines_type = new int[Lines.size()];

	float sum=0;
	for (int i=0;i<Lines.size();i++)
	{
		if (Lines_segments[i].size()>1){Lines_type[i]=2;}
		else
		{
			int seg_left=boxes[Lines_segments[i][0][0]][0];
			int seg_right=boxes[Lines_segments[i][0][Lines_segments[i][0].size()-1]][2];
			//if ((seg_right-seg_left)>=(page_right-page_left)/2){Lines_type[i]=1;}
			if ((seg_right-seg_left)>=(float)(page_right-page_left)/2.5){Lines_type[i]=1;}
			else if (seg_left >= (page_right-page_left)/4) {Lines_type[i]=2;}
			else {Lines_type[i]=3;}
		}

		if (i>1 && Lines_type[i]==2 && Lines_type[i-2]==2) Lines_type[i-1]=2;


		/*if (Lines_segments[i].size()==2)
		{
			int seg_left1=boxes[Lines_segments[i][0][0]][0];
			int seg_right1=boxes[Lines_segments[i][0][Lines_segments[i][0].size()-1]][2];
			int seg_left2=boxes[Lines_segments[i][1][0]][0];
			int seg_right2=boxes[Lines_segments[i][1][Lines_segments[i][1].size()-1]][2];
			if (((seg_right1-seg_left1)>=(page_right-page_left)/2)||
				((seg_right2-seg_left2)>=(page_right-page_left)/2))
			{Lines_type[i]=3;}
		}*/
		sum=sum+Line_dims[i][1]-Line_dims[i][0];
	}
	sum=(float)sum/Lines.size();


	/*// FOOTER/HEADER - Line height must be smaller than thw average line height

	// Header must begin from the first line, can have a max of 5 lines, header-lines must be consequential
	float ratio=0.95;
	for (int i=0;i<(std::min((int)Lines.size(),2));i++)
	{
		if ((Line_dims[i][1]-Line_dims[i][0])<=ratio*sum)
		{
			if (i==0)
			{
				Lines_type[i]=4;
			}
			else if (Lines_type[i-1]==4)
			{
				Lines_type[i]=4;
			}
			else {i=Lines.size();}
		}
	}

	// Footer must begin from the last line, footer-lines must be consequential

	for (int i=(Lines.size()-1);i>=0;i--)
	{
		if ((Line_dims[i][1]-Line_dims[i][0])<=ratio*sum)
		{
			if (i==Lines.size()-1)
			{
				Lines_type[i]=4;
			}
			else if (Lines_type[i+1]==4)
			{
				Lines_type[i]=4;
			}
			else {i=-1;}
		}
	}
	*/
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TableAreas()
	// Find areas that can potentially be real tables.
	// Such areas include consequential type-2 and type-3 lines.
{
	cout << "Find table areas...";
	start = clock();
	vector<int> tmp;
	
	if (Lines_type[0]!=1){tmp.push_back(0);}
	for (int i=1;i<Lines.size();i++)
	{
		if ((Lines_type[i]!=1)&&(Lines_type[i]!=4))
		{
			if ((Lines_type[i-1]!=1)&&(Lines_type[i-1]!=4)&&((Line_dims[i][0]-Line_dims[i-1][1])<=3*(Line_dims[i][1]-Line_dims[i][0])))
			{
				tmp.push_back(i);
			}
			else
			{
				if ((tmp.size()>1)||((tmp.size()==1)&&(Lines_type[tmp[0]]==2)))
				{
					table_area.push_back(tmp);				
				}
				tmp.clear();
				tmp.push_back(i);
			}
		}
	}
	if ((tmp.size()>1)||((tmp.size()==1)&&(Lines_type[tmp[0]]==2))){
		table_area.push_back(tmp);}
	tmp.clear();
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TableRows()
	// Assign Rows to tables. Each table must start with a type-2 line. So if there are initially type-3
	// without a type-2 line over them, they are not assigned to the table, unless they are not left-aligned
{
	cout << "Find table rows...";
	start = clock();
	for (int i=0;i<table_area.size();i++)
	{
		bool t_flag=false;
		vector<int> tmp;
		for (int j=0;j<table_area[i].size();j++)
		{
			if ((Lines_type[table_area[i][j]]==2)||(t_flag))
			{
				t_flag=true;
				tmp.push_back(table_area[i][j]);
			}
		}
		if (tmp.size()>0){table_Rows.push_back(tmp);}
	}

	// Tables that end up with just one Row are dismissed
	for (int i=0;i<table_Rows.size();i++)
	{
		if (table_Rows[i].size()<2)
		{
			Lines_type[table_Rows[i][0]]=1;
			table_Rows.erase(table_Rows.begin()+i);
		}
	}
	
	
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TableColumns()
	// Assign Columns to each table
{
	cout << "Find table columns...";
	start = clock();
	for (int i=0;i<table_Rows.size();i++)
	{
		// Find a segment to initalize columns
		// First we select the left-most segment
		// Then we find all the segments that almost vertically align (on the left) with this segment, and we find their average length
		// Then we choose as a column creator the segment that is
		// closest to the average length (or slightly bigger). We proceed by assigning to the final column all the segments that horizontally overlap with 
		// the column creator
		// Next we select the left-most segment that is not a part of the previous columns 
		// and we repeat the same process to create the next column
		// Some large segments can be assigned to more than one columns
		vector<vector<int>> column_creator;
		int limit=-1;
		vector<int> min;
		min=Lines_segments[table_Rows[0][0]][0];
		bool end_of_table=false;

		//FIND COLUMN GENERATORS
		while(!end_of_table){

		duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
		if (duration>30)
		{
			end_of_table=true;
			i=table_Rows.size();
			fail=true;
			cout<<"TASK FAILED\n";
			break;
		}
	
	
		for (int j=0;j<table_Rows[i].size();j++)
		{
			for (int k=0;k<Lines_segments[table_Rows[i][j]].size();k++)
			{
				int left=boxes[Lines_segments[table_Rows[i][j]][k][0]][0];
				int right=boxes[Lines_segments[table_Rows[i][j]][k][Lines_segments[table_Rows[i][j]][k].size()-1]][2];

				if ((left<=boxes[min[0]][0])&&(left>limit))
				{
					min=Lines_segments[table_Rows[i][j]][k];
				}
			}
		}
		//cout<<"\n\n"<<words[min[0]]<<"\n\n";
		float avg=0;
		int counter=0;
		int hor_thresh=(Line_dims[table_Rows[i][(int)table_Rows[i].size()/2]][1]-Line_dims[table_Rows[i][(int)table_Rows[i].size()/2]][0])*2.6;
		for (int j=0;j<table_Rows[i].size();j++)
		{
			for (int k=0;k<Lines_segments[table_Rows[i][j]].size();k++)
			{
				int left=boxes[Lines_segments[table_Rows[i][j]][k][0]][0];
				int right=boxes[Lines_segments[table_Rows[i][j]][k][Lines_segments[table_Rows[i][j]][k].size()-1]][2];
			
				if ((abs(left-boxes[min[0]][0])<=hor_thresh)&&
					/*((right-left)<=(boxes[min[min.size()-1]][2]-boxes[min[0]][0]))&&*/
					(left>limit))
				{
					//cout<<words[Lines_segments[table_Rows[i][j]][k][0]]<<"\n";
					//min=Lines_segments[table_Rows[i][j]][k];
					avg=avg+right-left;
					counter++;
				}
			}
		}
		int fin_left=boxes[min[0]][0];
		avg=(float)avg/counter;
		float overlap_ratio=1.2; //1.2
		avg=avg*overlap_ratio;
		for (int j=0;j<table_Rows[i].size();j++)
		{
			for (int k=0;k<Lines_segments[table_Rows[i][j]].size();k++)
			{
				int left=boxes[Lines_segments[table_Rows[i][j]][k][0]][0];
				int right=boxes[Lines_segments[table_Rows[i][j]][k][Lines_segments[table_Rows[i][j]][k].size()-1]][2];
			
				if ((abs(left-fin_left)<=hor_thresh)&&
					(abs(right-left-avg)<=abs(boxes[min[min.size()-1]][2]-boxes[min[0]][0]-avg))&&
					(left>limit))
				{
					min=Lines_segments[table_Rows[i][j]][k];
				}
			}
		}
		//cout<<"\n"<<words[min[0]]<<"\n\n";
		//int aas;
		//cin>>aas;
		column_creator.push_back(min);
		limit=boxes[min[min.size()-1]][2];
		min.clear();
		for (int j=0;j<table_Rows[i].size();j++)
		{
			for (int k=0;k<Lines_segments[table_Rows[i][j]].size();k++)
			{
				int left=boxes[Lines_segments[table_Rows[i][j]][k][0]][0];
				if (left>limit)
				{
					min=Lines_segments[table_Rows[i][j]][k];
					break;
				}
			}
		}
		if(min.size()==0){end_of_table=true;}
	}
		tmp_col.push_back(column_creator);
	}
	if (!fail){
	//FIND REST OF COLUMNS
	for (int i=0;i<tmp_col.size();i++)
	{
		vector<vector<vector<int>>> t_col;
		for (int j=0;j<tmp_col[i].size();j++)
		{
			int col_left=boxes[tmp_col[i][j][0]][0];
			int col_right=boxes[tmp_col[i][j][tmp_col[i][j].size()-1]][2];
			vector<vector<int>> t_seg;
			for (int k=0;k<table_Rows[i].size();k++)
			{
				for (int z=0;z<Lines_segments[table_Rows[i][k]].size();z++)
				{
					int seg_left=boxes[Lines_segments[table_Rows[i][k]][z][0]][0];
					int seg_right=boxes[Lines_segments[table_Rows[i][k]][z][Lines_segments[table_Rows[i][k]][z].size()-1]][2];
					if (((seg_right>=col_left)&&(seg_right<=col_right))||
						((seg_left>=col_left)&&(seg_left<=col_right))||
						((seg_left<=col_left)&&(seg_right>=col_right)))
					{
						t_seg.push_back(Lines_segments[table_Rows[i][k]][z]);
						
					}
				}
			}
			
			t_col.push_back(t_seg);
		}
		table_Columns.push_back(t_col);
	}

	if (!fail){

	for (int i=0;i<table_Columns.size();i++)
	{
		for (int j=0;j<table_Columns[i].size();j++)
		{
			if (table_Columns[i][j].size()==0)
			{
				table_Columns[i].erase(table_Columns[i].begin()+j);
				j--;
			}
		}
	}

	// If we find a column where the unique segments  (segments that are assigned to only one column)
	// are less than the multiple segemnts (segments assigned to more than 1 column), we merge this column with
	// the immediatelly previous one

	
	for (int i=0;i<table_Columns.size();i++)
	{
		for (int j=1;j<table_Columns[i].size();j++)
		{
			int counter_single=0;
			int counter_multi=0;
			for (int k=0;k<table_Columns[i][j].size();k++)
			{
				bool multi=false;
				if (std::find(table_Columns[i][j-1].begin(),table_Columns[i][j-1].end(),table_Columns[i][j][k])!=table_Columns[i][j-1].end())
				{
					multi=true;
				}
				if (multi)
				{
					counter_multi++;
				}
				else
				{
					counter_single++;
				}
			}
			if (counter_multi>=1*counter_single)
			{
				for (int k=0;k<table_Columns[i][j].size();k++)
				{
					bool multi=false;
					if (std::find(table_Columns[i][j-1].begin(),table_Columns[i][j-1].end(),table_Columns[i][j][k])!=table_Columns[i][j-1].end())
					{
						multi=true;
					}
					if (!multi){table_Columns[i][j-1].push_back(table_Columns[i][j][k]);}
				}
				table_Columns[i].erase(table_Columns[i].begin()+j);
				j--;
			}
		}
	}


	// Type-3 lines that are in the end of a table, and their single segment is assigned to more than one columns,
	// are removed from the table
	for (int i=0;i<table_Rows.size();i++)
	{
		for (int j=table_Rows[i].size()-1;j>=0;j--)
		{
			//cout << table_Rows[0].size()<<"     "<<table_Rows[1].size()<<"\n";
			//cout << i<<"     "<<j<<"\n";
			if (Lines_type[table_Rows[i][j]]==3)
			{
				vector<int> xcol;
				for (int k=0;k<table_Columns[i].size();k++)
				{
					if (table_Columns[i][k].size()>0){
					if (Lines_segments[table_Rows[i][j]][0]==table_Columns[i][k][table_Columns[i][k].size()-1])
					{xcol.push_back(k);}}
				}
				if (xcol.size()>1)
				{
					Lines_type[table_Rows[i][j]]=1;
					table_Rows[i].erase(table_Rows[i].begin()+j);
					for (int k=0;k<xcol.size();k++)
					{
						table_Columns[i][xcol[k]].erase(table_Columns[i][xcol[k]].begin()+table_Columns[i][xcol[k]].size()-1);
					}
				}
				else
				{
					j=-1;
				}
			}
			else
			{
				j=-1;
			}
		}
	}

	//If a column has only one segment, which is on the 1st row (possibly missaligned table header) and the column on its left doesnot have a segment
	// in the same row, then merge these columns
	for (int i=0;i<table_Columns.size();i++)
	{
		for (int j=1;j<table_Columns[i].size();j++)
		{
			if (table_Columns[i][j].size()==1)
			{
				bool found1=false;
				bool found2=false;
				if (std::find(Lines_segments[table_Rows[i][0]].begin(),Lines_segments[table_Rows[i][0]].end(),table_Columns[i][j][0])!=Lines_segments[table_Rows[i][0]].end())
				{found1=true;}
				if (std::find(Lines_segments[table_Rows[i][0]].begin(),Lines_segments[table_Rows[i][0]].end(),table_Columns[i][j-1][0])!=Lines_segments[table_Rows[i][0]].end())
				{found2=true;}
				if ((found1)&&(!found2))
				{
					for (int k=0;k<table_Columns[i][j-1].size();k++)
					{
						table_Columns[i][j].push_back(table_Columns[i][j-1][k]);
					}
					table_Columns[i].erase(table_Columns[i].begin()+j-1);
				}
			}
		}
	}

	// Tables that end up having only one column, are discarded and treated as simple text
	for (int i=table_Columns.size()-1;i>=0;i--)
	{
		if (table_Columns[i].size()<2)
		{
			table_Columns.erase(table_Columns.begin()+i);
			for (int j=0;j<table_Rows[i].size();j++)
			{
				Lines_type[table_Rows[i][j]]=1;
			}
			table_Rows.erase(table_Rows.begin()+i);
		}
	}

	//Check and remove scrambled tables i.e tables that seem to have a table format but in reality are random segments generated by
	// big white spaces bwtween words (justified text aligment)

	//bool scrambled_table=false;
	//int scramLim = 0;
	//for (int i=table_Columns.size()-1;i>=0;i--)
	//{
	//	if (scrambled_table)
	//	{	
	//		table_Columns.erase(table_Columns.begin()+i+1);
	//		for (int k=0;k<table_Rows[i+1].size();k++)
	//		{
	//			Lines_type[table_Rows[i+1][k]]=1;
	//		}
	//		table_Rows.erase(table_Rows.begin()+i+1);
	//	}
	//	scrambled_table=false;
	//	for (int j=0;j<table_Columns[i].size();j++)
	//	{
	//		bool scrambled_col=false;
	//		for (int k=0;k<table_Columns[i][j].size()-1;k++)
	//		{
	//			int left1=boxes[table_Columns[i][j][k][0]][0];
	//			int left2=boxes[table_Columns[i][j][k+1][0]][0];
	//			int right1=boxes[table_Columns[i][j][k][table_Columns[i][j][k].size()-1]][2];
	//			int right2=boxes[table_Columns[i][j][k+1][table_Columns[i][j][k+1].size()-1]][2];	
	//			if ((abs(left1-left2)<=scramLim)||(abs(right1-right2)<=scramLim))
	//			{
	//				scrambled_col=true;
	//			}
	//			if (k<table_Columns[i][j].size()-2){
	//			left2=boxes[table_Columns[i][j][k+2][0]][0];
	//			right2=boxes[table_Columns[i][j][k+2][table_Columns[i][j][k+2].size()-1]][2];	
	//			if ((abs(left1-left2)<=scramLim)||(abs(right1-right2)<=scramLim))
	//			{
	//				scrambled_col=true;
	//			}
	//			}
	//		}
	//		if (!scrambled_col)
	//		{
	//			scrambled_table=true;
	//			j=table_Columns[i].size();
	//		}
	//	}
	//}
	//if (scrambled_table)
	//{	
	//	table_Columns.erase(table_Columns.begin()+0);
	//	for (int k=0;k<table_Rows[0].size();k++)
	//	{
	//		Lines_type[table_Rows[0][k]]=1;
	//	}
	//	table_Rows.erase(table_Rows.begin()+0);
	//}

	//If all the columns of a table (besides the 1st one) have more empty cells than cells with data then
	//this table is discarded (simple formatted text)
	for (int i=table_Columns.size()-1;i>=0;i--)
	{
		bool almost_empty=false;
		for (int j=1;j<table_Columns[i].size();j++)
		{
			if (table_Columns[i][j].size()<table_Rows[i].size()/2)
			{
				almost_empty=true;
			}
			else
			{
				almost_empty=false;
				j=table_Columns[i].size();
			}
		}
		if (almost_empty)
		{
			table_Columns.erase(table_Columns.begin()+i);
			for (int k=0;k<table_Rows[i].size();k++)
			{
				Lines_type[table_Rows[i][k]]=1;
			}
			table_Rows.erase(table_Rows.begin()+i);
		}
	}


	
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
	}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::TableMultiRows()
	// create table rows that include more than one lines
{
	cout << "Find table multiple-rows...";
	start = clock();
	// if a table line does not have a segment in the first column, and there is one-to-one column correspondence 
	// with the line above it, 
	// it is merged with the one above it
	// this does not apply for the first line of the table
	for (int i=0;i<table_Rows.size();i++)
	{
		vector<vector<int>> tmp_multi_row;
		vector<int> tmp_multi_lines;
		tmp_multi_lines.push_back(table_Rows[i][0]);
		for (int j=1;j<table_Rows[i].size();j++)
		{
			bool found=false;
			if (std::find(table_Columns[i][0].begin(),table_Columns[i][0].end(),Lines_segments[table_Rows[i][j]][0])!=table_Columns[i][0].end())
			{found=true;}
			if (!found)
			{
				bool exist_all=true;
				for (int k=0;k<Lines_segments[table_Rows[i][j]].size();k++)
				{
					bool exist=false;
					for(int s=0;s<table_Columns[i].size();s++)
					{	
						bool exist1=false;bool exist0=false;
						for (int h=0;h<table_Columns[i][s].size();h++)
						{	
							if (Lines_segments[table_Rows[i][j]][k]==table_Columns[i][s][h])
							{
								exist1=true;
							}
							for (int z=0;z<Lines_segments[table_Rows[i][j-1]].size();z++)
							{
								if (Lines_segments[table_Rows[i][j-1]][z]==table_Columns[i][s][h])
								{
									exist0=true;
									if ((k<Lines_segments[table_Rows[i][j]].size()-1)&&
										(boxes[Lines_segments[table_Rows[i][j-1]][z][Lines_segments[table_Rows[i][j-1]][z].size()-1]][2]>=boxes[Lines_segments[table_Rows[i][j]][k+1][0]][0]))
									{
										exist0=false;
									}
								}
							}
						}
						if ((exist1)&&(exist0)){s=table_Columns[i].size();exist=true;}
					}
					exist_all=(exist_all)&&(exist);
				}
				if (exist_all)
				{
					tmp_multi_lines.push_back(table_Rows[i][j]);
				}
				else
				{
					tmp_multi_row.push_back(tmp_multi_lines);
					tmp_multi_lines.clear();
					tmp_multi_lines.push_back(table_Rows[i][j]);
				}
			}
			else
			{
				tmp_multi_row.push_back(tmp_multi_lines);
				tmp_multi_lines.clear();
				tmp_multi_lines.push_back(table_Rows[i][j]);
			}
		}
		tmp_multi_row.push_back(tmp_multi_lines);
		multi_Rows.push_back(tmp_multi_row);
		tmp_multi_lines.clear();
		tmp_multi_row.clear();
		
		// if a line is type-3, its segment is assigned ONLY to the first column,
		// the line below it is type-2, and it has a segment in the fist column which is more to the
		// right than the segment of the first line THEN these two lines are merged together
		for (int j=0;j<multi_Rows[i].size()-1;j++)
		{
			if ((multi_Rows[i][j].size()==1)&&
				(Lines_type[multi_Rows[i][j][0]]==3)&&(Lines_segments[multi_Rows[i][j][0]].size()==1)&&
				(Lines_type[multi_Rows[i][j+1][0]]==2))
			{
				bool found1=false;
				bool found2=false;
				if (std::find(table_Columns[i][0].begin(),table_Columns[i][0].end(),Lines_segments[multi_Rows[i][j][0]][0])!=table_Columns[i][0].end())
				{found1=true;}
				if (std::find(table_Columns[i][0].begin(),table_Columns[i][0].end(),Lines_segments[multi_Rows[i][j+1][0]][0])!=table_Columns[i][0].end())
				{found2=true;}
				int tmp=Lines_segments[multi_Rows[i][j][0]][0][0];
				float lft1=boxes[tmp][0];
				tmp=Lines_segments[multi_Rows[i][j+1][0]][0][0];
				float lft2=boxes[tmp][0];
				if ((found1)&&(found2)&&(lft2>=lft1+0.6*(Line_dims[multi_Rows[i][j+1][0]][1]-Line_dims[multi_Rows[i][j+1][0]][0])))
				{
					for (int z=0; z<multi_Rows[i][j+1].size();z++)
					{
						multi_Rows[i][j].push_back(multi_Rows[i][j+1][z]);
					}
					multi_Rows[i].erase(multi_Rows[i].begin()+j+1);
				}
			}
		}
		if ((multi_Rows[i][multi_Rows[i].size()-1].size()==1)&&
			(Lines_type[multi_Rows[i][multi_Rows[i].size()-1][0]]==3)&&(Lines_segments[multi_Rows[i][multi_Rows[i].size()-1][0]].size()==1)&&
			(Lines_segments[multi_Rows[i][multi_Rows[i].size()-1][0]][0]==table_Columns[i][0][table_Columns[i][0].size()-1]))
		{
			Lines_type[multi_Rows[i][multi_Rows[i].size()-1][0]]=1;
			multi_Rows[i].erase(multi_Rows[i].begin()+multi_Rows[i].size()-1);
			table_Columns[i][0].erase(table_Columns[i][0].begin()+table_Columns[i][0].size()-1);
		}



		//Finalize Line Types. Change all Lines within the table to type-2
		for (int j=0;j<multi_Rows[i].size();j++)
		{
			for (int k=0;k<multi_Rows[i][j].size();k++)
			{
				Lines_type[multi_Rows[i][j][k]]=2;
			}
		}
	}
	
	//Recheck and discard single and double row tables 
	for (int i=multi_Rows.size()-1;i>=0;i--)
	{
		if ((multi_Rows[i].size()<2)&&(multi_Rows[i][0].size()<2 || table_Columns[i][0].size()<2))
		{
			for (unsigned s=0;s<multi_Rows[i][0].size();s++) {Lines_type[multi_Rows[i][0][s]]=1;}
			multi_Rows.erase(multi_Rows.begin()+i);
			table_Columns.erase(table_Columns.begin()+i);
		}
		else if ((multi_Rows[i].size()<3)&&(multi_Rows[i][0].size()<2)&&(multi_Rows[i][1].size()<2))
		{
			for (unsigned s=0;s<multi_Rows[i][0].size();s++) {Lines_type[multi_Rows[i][0][s]]=1;}
			for (unsigned s=0;s<multi_Rows[i][1].size();s++) {Lines_type[multi_Rows[i][1][s]]=1;}
			multi_Rows.erase(multi_Rows.begin()+i);
			table_Columns.erase(table_Columns.begin()+i);
		}
	}


		duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";

}
//////////////////////////////////////////////////////////////
void ocr_tabs::ColumnSize()
	//find the sizes of the columns. Each columns spans from the leftest single segment to the rightest
	// single segment (single segment = assigned to only one column)
{
	cout << "Find column sizes...";
	start = clock();
	for (int i=0;i<table_Columns.size();i++)
	{
		vector<int*> dims;
		for (int j=0;j<table_Columns[i].size();j++)
		{
			int* tmp = new int[2];
			tmp[0]=page_right;
			tmp[1]=page_left;
			for (int k=0;k<table_Columns[i][j].size();k++)
			{
				bool flag_left=true;
				bool flag_right=true;
				if (j>=1)
				{
					if(std::find(table_Columns[i][j-1].begin(),table_Columns[i][j-1].end(),table_Columns[i][j][k])!=table_Columns[i][j-1].end())
					{	
						flag_left=false;
					}												
				}
				if (j<table_Columns[i].size()-1)
				{
					if (std::find(table_Columns[i][j+1].begin(),table_Columns[i][j+1].end(),table_Columns[i][j][k])!=table_Columns[i][j+1].end())
					{
						flag_right=false;
					}
				}
				if ((boxes[table_Columns[i][j][k][0]][0]<=tmp[0])&&(flag_left))
				{tmp[0]=boxes[table_Columns[i][j][k][0]][0];}	
				if ((boxes[table_Columns[i][j][k][table_Columns[i][j][k].size()-1]][2]>=tmp[1])&&(flag_right))
				{tmp[1]=boxes[table_Columns[i][j][k][table_Columns[i][j][k].size()-1]][2];}
			}
			dims.push_back(tmp);
		}
		col_dims.push_back(dims);
	}

	for (int i=table_Columns.size()-1;i>=0;i--)
	{
		if (table_Columns[i].size() == 2)
		{
			int colSizeA = col_dims[i][0][1] - col_dims[i][0][0];
			int colSizeB = col_dims[i][1][1] - col_dims[i][1][0];
			if (colSizeB >= 10*colSizeA)
			{
				table_Columns.erase(table_Columns.begin()+i);
				for (int j=0;j<multi_Rows[i].size();j++)
				{
					for (int k=0;k<multi_Rows[i][j].size();k++)
					{
						Lines_type[multi_Rows[i][j][k]]=1;
					}
				}
				//table_Rows.erase(table_Rows.begin()+i);
				multi_Rows.erase(multi_Rows.begin()+i);
				col_dims.erase(col_dims.begin()+i);
			}
		}
	}

	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::FinalizeGrid()
	// find the final row and column sizes in order to create the table
	// merge cells that contain multi segments
	// widen columns and rows so that there is no white space between them
{
	cout << "Finalize grid...";
	start = clock();
	for (int i=0;i<col_dims.size();i++)
	{
		for (int j=1;j<col_dims[i].size();j++)
		{
			col_dims[i][j][0]=(col_dims[i][j][0]+col_dims[i][j-1][1])/2;
			col_dims[i][j-1][1]=col_dims[i][j][0];
		}
	}
	for (int i=0;i<multi_Rows.size();i++)
	{
		vector<int*> dims;
		for (int j=0;j<multi_Rows[i].size();j++)
		{
			int* tmp=new int[2];
			tmp[0]=Line_dims[multi_Rows[i][j][0]][0];
			tmp[1]=Line_dims[multi_Rows[i][j][multi_Rows[i][j].size()-1]][1];
			dims.push_back(tmp);
			if (j>0)
			{
				dims[j][0]=(dims[j][0]+dims[j-1][1])/2;
				int low=dims[j][0];
				dims[j-1][1]=low;			
			}
		}
		row_dims.push_back(dims);
	}
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawBoxes()
{
	namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<boxes.size();i++)
	{
		int top=boxes[i][1];
		int bottom=boxes[i][3];
		int left=boxes[i][0];
		int right=boxes[i][2];
		line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
		line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
		cout <<"\n" << words[i] << "\t\t" << confs[i]<< "  " << font_size[i]<<" ";
		if (bold[i]){cout<<"bold  ";}
		if (italic[i]){cout<<"italic  ";}
		if (underscore[i]){cout<<"underlined  ";}
		if (dict[i]){cout<<"in dictionary  ";}
		cout << boxes[i][0] << "|"<< boxes[i][1] << "|"<< boxes[i][2] << "|"<< boxes[i][3];
		cout<<"\n";
		imshow("img", test);
		char c=cvWaitKey(0);
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawLines()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<Lines.size();i++)
	{
		line( test, Point2i( page_left,Line_dims[i][0]),Point2i( page_right,Line_dims[i][0]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_right,Line_dims[i][0]),Point2i( page_right,Line_dims[i][1]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_right,Line_dims[i][1]),Point2i( page_left,Line_dims[i][1]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_left,Line_dims[i][1]),Point2i( page_left,Line_dims[i][0]), Scalar (0,0,0), 2 );
		imshow("img", test);
	char c=cvWaitKey(0);
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawSegments()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<Lines.size();i++)
	{
		for (int j=0;j<Lines_segments[i].size();j++)
		{
			int top=Line_dims[i][0];
			int bottom=Line_dims[i][1];
			int left=boxes[Lines_segments[i][j][0]][0];
			int right=boxes[Lines_segments[i][j][Lines_segments[i][j].size()-1]][2];
			line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
			line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
			line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
			line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
			imshow("img", test);
			char c=cvWaitKey(0);
		}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawAreas()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<table_area.size();i++)
	{
		int top=Line_dims[table_area[i][0]][0];
		int bottom=Line_dims[table_area[i][table_area[i].size()-1]][1];
		int left=page_left;
		int right=page_right;
		line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
		line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
		imshow("img", test);
		char c=cvWaitKey(0);
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawRows()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<multi_Rows.size();i++)
	{
		for (int j=0;j<multi_Rows[i].size();j++)
		{

		int top=Line_dims[multi_Rows[i][j][0]][0];
		int bottom=Line_dims[multi_Rows[i][j][multi_Rows[i][j].size()-1]][1];
		int left=page_left;
		int right=page_right;
		line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
		line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
		line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
		imshow("img", test);
		char c=cvWaitKey(0);
		}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawColsPartial()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<tmp_col.size();i++)
	{
		for (int j=0;j<tmp_col[i].size();j++)
		{
				int top=boxes[tmp_col[i][j][0]][1];
				int bottom=boxes[tmp_col[i][j][0]][3];
				int left=boxes[tmp_col[i][j][0]][0];
				int right=boxes[tmp_col[i][j][tmp_col[i][j].size()-1]][2];
				line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
				line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
				imshow("img", test);
				char c=cvWaitKey(0);
		}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawCols()
{
		namedWindow("img", 0);
		float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<table_Columns.size();i++)
	{
		for (int j=0;j<table_Columns[i].size();j++)
		{
			for (int k=0;k<table_Columns[i][j].size();k++)
			{
				int tmp=table_Columns[i][j][k][0];
				int bottom=boxes[tmp][3];
				int top=boxes[tmp][1]; 
				int left=boxes[tmp][0];
				tmp=table_Columns[i][j][k][table_Columns[i][j][k].size()-1];
				int right=boxes[tmp][2];
				line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
				line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );
				imshow("img", test);
				char c=cvWaitKey(0);
			}
		}
	}
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawGrid()
{
	namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<col_dims.size();i++)
	{
		for (int j=0;j<row_dims[i].size();j++)
		{
			int top=row_dims[i][j][0];
			int bottom=row_dims[i][j][1];
			for (int k=0;k<col_dims[i].size();k++)
			{
				int left=col_dims[i][k][0];
				int right=col_dims[i][k][1];
				line( test, Point2i( left,top),Point2i( right,top), Scalar (0,0,0), 2 );
				line( test, Point2i( right,top),Point2i( right,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( right,bottom),Point2i( left,bottom), Scalar (0,0,0), 2 );
				line( test, Point2i( left,bottom),Point2i( left,top), Scalar (0,0,0), 2 );

			}
			for (int k=0;k<multi_Rows[i][j].size();k++)
			{
				for (int h=0;h<Lines_segments[multi_Rows[i][j][k]].size();h++)
				{
					int left=boxes[Lines_segments[multi_Rows[i][j][k]][h][0]][0];
					int right=boxes[Lines_segments[multi_Rows[i][j][k]][h][Lines_segments[multi_Rows[i][j][k]][h].size()-1]][2];
					int col_left=-1;
					int col_right=-1;
					for (int z=0;z<col_dims[i].size();z++)
					{
						if ((left<=col_dims[i][z][1])&&(col_left==-1))
						{col_left=z;}
						if ((right<=col_dims[i][z][1])&&(col_right==-1))
						{col_right=z;}
					}
					if (col_left!=col_right)
					{
						for (int z=col_left+1;z<=col_right;z++)
						{
							int point=col_dims[i][z][0];
							line( test, Point2i( point,top+(bottom-top)*0.1),Point2i( point,bottom), Scalar (255,255,255), 2 );
						}
					}
				}
			}
		}
	}
				imshow("img", test);
				char c=cvWaitKey(0);
}
//////////////////////////////////////////////////////////////
void ocr_tabs::DrawGridlessImage()
{
	namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	imshow("img",test);
	cvWaitKey(0);
}
//////////////////////////////////////////////////////////////
/*void ocr_tabs::DrawFootHead()
{
	namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	for (int i=0;i<Lines.size();i++)
	{
		if (Lines_type[i]==4){
		line( test, Point2i( page_left,Line_dims[i][0]),Point2i( page_right,Line_dims[i][0]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_right,Line_dims[i][0]),Point2i( page_right,Line_dims[i][1]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_right,Line_dims[i][1]),Point2i( page_left,Line_dims[i][1]), Scalar (0,0,0), 2 );
		line( test, Point2i( page_left,Line_dims[i][1]),Point2i( page_left,Line_dims[i][0]), Scalar (0,0,0), 2 );
		}
	}
	imshow("img", test);
	char c=cvWaitKey(0);
	
}*/

void ocr_tabs::ResetImage()
{
	test=initial.clone();
	//resize(test,test,Size(test.size().width*2,test.size().height*2));
}
//////////////////////////////////////////////////////////////
Mat ocr_tabs::ImgSeg(Mat img)
{
	//Search for multi column text
	cout << "Segment Image...";
	start = clock();
	Mat dst;
	threshold( img, img, 200, 255,0 );
	namedWindow("img", 0);
	vector<double> hor,ver;
	test=img;

	uchar* data = (uchar*)img.data;
	
	for (int i=0;i<img.rows;i++)
	{
		double sum=0;
		for (int j=0;j<img.cols;j++)
		{
			sum += data[j+i*img.cols];
		}
		hor.push_back(sum/img.cols);
	}
	for (int i=0;i<img.cols;i++)
	{
		double sum=0;
		for (int j=0;j<img.rows;j++)
		{
			sum += data[i+j*img.cols];
		}
		ver.push_back(sum/img.rows);
	}


	cvtColor(img,img,CV_GRAY2BGR);
	for (int i=0;i<hor.size();i++)
	{
			line(img,Point2i(0,i),Point2i(hor[i]*2-400,i),Scalar(0,0,255),2);
	}
		for (int i=0;i<ver.size();i++)
	{
			line(img,Point2i(i,0),Point2i(i,ver[i]*2-400),Scalar(0,255,0),2);
	}










	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	imshow("img",img);
	cvWaitKey(0);
	
	
	
	
	
	/*img=255-img;
	cv::distanceTransform(img, dst, CV_DIST_L2, 3);
			test=dst;
	namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	imshow("img",test);
	cvWaitKey(0);*/

	
	dst=img.clone();
	threshold( img, img, 200, 255,0 );
	erode(img,img,Mat(),Point(-1,-1),10);
	/*erode(img,img,Mat(),Point(1,1),20);
	img=255-img;

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	Mat SegMap = Mat::zeros(img.size().height, img.size().width,  CV_8UC1);
		if (contours.size()){
			int idx = 0;
			for( ; idx >= 0; idx = hierarchy[idx][0] )
			{				
				if (contours[idx].size()>=1) 
				{	Scalar color = Scalar( 255,255,255 );
					drawContours( SegMap, contours, idx, color, -1, 4, hierarchy, 0);
				}
			}
		}




	test=SegMap;
		namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	imshow("img",test);
	cvWaitKey(0);*/
	



	img=255-img;
	data = (uchar*)img.data;
	vector<int> red;
	/*cvtColor(dst,dst,CV_GRAY2BGR);
	
	
	for (int i=0;i<img.cols;i++)
	{
		double sum=0;
		for (int j=0;j<img.rows;j++)
		{
			sum += data[i+j*img.cols];
		}
		if (sum<50*255)
		{
			line(dst,Point2i(0,i),Point2i(img.cols-1,i),Scalar(0,0,255),2);
		}
	}

	test=dst;
	namedWindow("img", 0);
	float ratio=(float)(std::max(test.cols,test.rows))/850;
	resizeWindow("img",(test.cols)/ratio,(test.rows)/ratio);
	imshow("img",test);
	cvWaitKey(0);*/












	for (int i=0;i<img.cols;i++)
	{
		double sum=0;
		for (int j=0;j<img.rows;j++)
		{
			sum += data[i+j*img.cols];
		}
		if (sum<255*25)
		{
			red.push_back(i);
		}
	}
	vector<int> text_columns;
	for (int i=1;i<red.size();i++)
	{
		if (((red[i]-red[i-1])>1))
		{
			text_columns.push_back(red[i-1]);
			text_columns.push_back(red[i]);
		}
	}
	bool cols=true;
	float col_size_thresh=0.1;
	if ((text_columns.size()>3)&&(text_columns.size()%2==0))
	{
		for (int i=0;i<text_columns.size()-2;i=i+2)
		{
			for (int j=i+2;j<text_columns.size()-1;j=j+2)
			{
				int tmp = abs((text_columns[i+1]-text_columns[i])-(text_columns[j+1]-text_columns[j]));
				if (tmp>=col_size_thresh*(text_columns[i+1]-text_columns[i]))
				{
					cols=false;
				}
			}
		}
	}
	else
	{
		cols=false;
	}
	if (cols)
	{
		Mat single_col= Mat::ones(dst.rows*(text_columns.size()/2),(text_columns[1]-text_columns[0])*(1+col_size_thresh),dst.type())*255;
		for (int i=0;i<text_columns.size();i=i+2)
		{
			Mat tmp(single_col, Rect(0, i*dst.rows/2, text_columns[i+1]-text_columns[i], dst.rows));
			Mat tmp2(dst, Rect(text_columns[i],0, text_columns[i+1]-text_columns[i],dst.rows));
			tmp2.copyTo(tmp);
		}
		duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
		cout << " Done in "<<duration<< "s \n";
		return single_col;
	}
	
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";		
	return dst;
}
//////////////////////////////////////////////////////////////
void ocr_tabs::WriteHTML(std::string& filename)
{
	cout << "Create HTML...";
	start = clock();

	//find average font size
	vector<int> tmp_size;

	tmp_size=font_size;

	size_t n = tmp_size.size() / 2;
    nth_element(tmp_size.begin(), tmp_size.begin()+n, tmp_size.end());
	int font_size_avg=tmp_size[n];
	float ratio=(float)12/font_size_avg;
	font_size_avg=12;
	tmp_size[0]=1;
	tmp_size[1]=2;

	vector<vector<string>> font;
	for (int i=0;i<font_size.size();i++)
	{	//font_size[i]=font_size[i]*ratio;
		vector<string> tmp;
		//if (font_size[i]<(font_size_avg-3))
		//{
		//	tmp.push_back("<font size=\"1\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg-2))
		//{
		//	tmp.push_back("<font size=\"2\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+2))
		//{
			tmp.push_back("");
			tmp.push_back("");
		//}
		//else if (font_size[i]<(font_size_avg+4))
		//{
		//	tmp.push_back("<font size=\"4\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+6))
		//{
		//	tmp.push_back("<font size=\"5\">");
		//	tmp.push_back("</font>");
		//}
		//else if (font_size[i]<(font_size_avg+8))
		//{
		//	tmp.push_back("<font size=\"6\">");
		//	tmp.push_back("</font>");
		//}
		//else
		//{
		//	tmp.push_back("<font size=\"7\">");
		//	tmp.push_back("</font>");
		//}
		/*if (bold[i])
		{
			tmp[0].append("<b>");
			tmp[1].append("</b>");
		}
		if (italic[i])
		{
			tmp[0].append("<i>");
			tmp[1].append("</i>");
		}
		if (underscore[i])
		{
			tmp[0].append("<u>");
			tmp[1].append("</u>");
		}*/
		font.push_back(tmp);
	}
	std::ofstream file (filename);
	int table_num=0;
	if (file.is_open())
	{
		file << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n<html>\n<body>\n";
		file << "<p>\n";
		for (int x=0;x<Lines_segments.size();x++)
		{ 
			if (Lines_type[x]!=2)
			{
				for (int j=0;j<Lines_segments[x].size();j++)
				{
					for (int k=0;k<Lines_segments[x][j].size();k++)
					{
						file <<font[Lines_segments[x][j][k]][0]<< words[Lines_segments[x][j][k]] <<" "<<font[Lines_segments[x][j][k]][1];
					}
				}
				file << "<br>";
			}
			else
			{
				file << "\n</p>\n";
				file << "<table border=\"1\">\n";
				for (int i=0;i<multi_Rows[table_num].size();i++)
				{
					file << "<tr>\n";
					vector<vector<vector<int>>> col_tmp;
					for (int j=0;j<table_Columns[table_num].size();j++)
					{
						vector<vector<int>> ctmp;
						col_tmp.push_back(ctmp);
					}
					for (int j=0;j<multi_Rows[table_num][i].size();j++)
					{
						for (int k=0;k<Lines_segments[multi_Rows[table_num][i][j]].size();k++)
						{
							for (int s=0;s<table_Columns[table_num].size();s++)
							{
								if (std::find(table_Columns[table_num][s].begin(),table_Columns[table_num][s].end(),Lines_segments[multi_Rows[table_num][i][j]][k])!=table_Columns[table_num][s].end())
								{
									col_tmp[s].push_back(Lines_segments[multi_Rows[table_num][i][j]][k]);
								}
							}
						}
					}
					for (int j=0;j<table_Columns[table_num].size();j++)
					{
						file << "<td";
						int colspan=1;
						if (col_tmp[j].size()==0)
						{
							file<< ">";
						}
						else
						{
							for (int h=j+1;h<table_Columns[table_num].size();h++)
							{
								if (col_tmp[h].size()!=0)
								{
									for (int aa=0;aa<col_tmp[j].size();aa++)
									{
										if (std::find(col_tmp[h].begin(),col_tmp[h].end(),col_tmp[j][aa])!=col_tmp[h].end())
										{
											colspan++;
											aa=col_tmp[j].size();
										}
									}
								}
							}
							if (colspan>1)
							{
								file <<" colspan=\""<<colspan<<"\"";
								for (int h=j+1;h<j+colspan;h++)
								{
									for (int aa=0;aa<col_tmp[h].size();aa++)
									{
										bool found=false;
										if (std::find(col_tmp[j].begin(),col_tmp[j].end(),col_tmp[h][aa])!=col_tmp[j].end())
										{
											found=true;
										}
										if (!found){col_tmp[j].push_back(col_tmp[h][aa]);}
									}
								}
							}
							file<< ">";
						}
						for (int k=0;k<col_tmp[j].size();k++)
						{
							for (int s=0;s<col_tmp[j][k].size();s++)
							{
								file <<font[col_tmp[j][k][s]][0]<< words[col_tmp[j][k][s]] <<" "<<font[col_tmp[j][k][s]][1];
							}
							if (col_tmp[j].size()>(k+1))
							{
								file << "<br>";
							}
						}
						file << "</td>\n";
						j=j+colspan-1;
					}
					file << "</tr>\n";
				}
				file << "</table>\n<p>\n";
				while (Lines_type[x]==2){x++;}
				x--;
				table_num++;
			}
		}
		file << "\n</p>\n</body>\n</html>";
		file.close();
		duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
		cout << " Done in "<<duration<< "s \n";
	}
	else
	{cout << "Unable to open file";}
}
//////////////////////////////////////////////////////////////
Mat ocr_tabs::ImagePreproccesing(Mat img)
{
	cout<< "Process Image...";
	start = clock();

	test = cv::Mat(img);

	float ratio;

	if (((std::max(img.size().width,img.size().height))<4200)&&((std::max(img.size().width,img.size().height))>2800))
	{
		ratio = 1;
	}
	else if (img.size().width>img.size().height)
	{
		ratio = (float)img.size().width/3500;
	}
	else
	{
		ratio = (float)img.size().height/3500;
	}
	RemoveGridLines(ratio);
	imgProcessor::segmentationBlocks blk;
	cv::Mat clean,clean2;
	imgProcessor::prepareAll(img,clean, blk);

	if (ratio >= 0.8) cv::erode(clean,clean,cv::Mat(), cv::Point(-1,-1), 1);
	//cv::imshow("asd", clean);
	//cv::waitKey(0);
	imgProcessor::getTextImage(clean, blk, clean2);
	//imgProcessor::getTextImage(img, blk, clean2);
	cv::Size orgSiz = img.size();
	imgProcessor::reorderImage(clean2, blk, img);

	if (((std::max(orgSiz.width,orgSiz.height))<4200)&&((std::max(orgSiz.width,orgSiz.height))>2800))
	{
		duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
		cout << " Done in "<<duration<< "s \n";
		return (img);
	}
	if (orgSiz.width>orgSiz.height)
	{
		resize(img,img,cv::Size(3500,img.size().height/((float)img.size().width/3500)));
	}
	else
	{
		resize(img,img,cv::Size(img.size().width/((float)orgSiz.height/3500),3500*(float)img.size().height/orgSiz.height));
	}
	duration = ( std::clock() - start ) / CLOCKS_PER_SEC;
	cout << " Done in "<<duration<< "s \n";
	return (img);
}
//////////////////////////////////////////////////////////////
bool ocr_tabs::fail_condition(){return fail;}
//////////////////////////////////////////////////////////////
bool ocr_tabs::pdf2html (const std::string& filename)
{
	resetAll();
	std::vector<cv::Mat> pages;
	if (!parsePDF(filename, pages)) return false;
	if (pages.size()==1)
	//if (pages.size()!=1)
	{
		test=ImagePreproccesing(pages[0]);
		SetImage(test);
		//RemoveGridLines();
		OCR_Recognize();
		BoxesAndWords();
		TextBoundaries();
	}
	else
	{
		for (int i=0;i<pages.size();i++)
		{
			Mat tmp=ImagePreproccesing(pages[i]);
			SetImage(tmp);
			//RemoveGridLines();
			PrepareMulti1();
		}
		HeadersFooters();
		PrepareMulti2();
	}
	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition())
	{
		cout<<"\nfailCondition\n";
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append(".html");
	WriteHTML(outputFilename);
	return true;
}
//////////////////////////////////////////////////////////////
bool ocr_tabs::img2html (const std::string& filename)
{
	resetAll();
	Mat test=imread(filename,0);
	if (test.empty()) {cout << "File not available\n"; return false;}
	test=ImagePreproccesing(test);
	SetImage(test);
	//RemoveGridLines();
	OCR_Recognize();
	BoxesAndWords();
	TextBoundaries();
	TextLines();
	LineSegments();
	LineTypes();
	TableAreas();
	TableRows();
	TableColumns(); ////
	if (fail_condition())
	{
		cout<<"\nfailCondition\n";
		return false;
	}
	TableMultiRows();
	ColumnSize();	 ////
	FinalizeGrid(); ////
	std::string outputFilename = filename;
	outputFilename.append(".html");
	WriteHTML(outputFilename);
	return true;
}
//////////////////////////////////////////////////////////////
void ocr_tabs::resetAll()
{
	test = cv::Mat();
	initial = cv::Mat();
	words.clear();
	boxes.clear();
	Lines.clear();
	table_area.clear();
	table_Rows.clear();
	multi_Rows.clear();
	Line_dims.clear();
	Lines_segments.clear();
	table_Columns.clear();
	col_dims.clear();
	row_dims.clear();
	tmp_col.clear();
	confs.clear();
	bold.clear();
	dict.clear();
	italic.clear();
	underscore.clear();
	font_size.clear();
	if (Lines_type!=NULL) delete Lines_type;
	words_.clear();
	Lines_.clear();
	boxes_.clear();
	confs_.clear();
	font_size_.clear();
	bold_.clear();
	dict_.clear();
	italic_.clear();
	underscore_.clear();
	page_height.clear();
	page_width.clear();
}
//////////////////////////////////////////////////////////////
bool ocr_tabs::parsePDF(const std::string& filename, std::vector<cv::Mat>& imageList)
{
	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	// Register document handlers for the default file types we support.
	fz_register_document_handlers(ctx);
	// Open the PDF, XPS or CBZ document.
	fz_document *doc = fz_open_document(ctx, filename.c_str());
	// Retrieve the number of pages (not used in this example).
	int pagecount = fz_count_pages(ctx, doc);
	if (pagecount<1)
	{
		fz_drop_document(ctx,doc);
		fz_drop_context(ctx);
		return false;
	}
	int rotation = 0;
	float zoom = 400;
	for (unsigned i=0;i<pagecount;i++)
	{
		fz_page *page = fz_load_page(ctx, doc, i);
		fz_matrix transform;
		fz_rotate(&transform, rotation);
		fz_pre_scale(&transform, zoom / 100.0f, zoom / 100.0f);
		fz_rect bounds;
		fz_bound_page(ctx, page, &bounds);
		fz_transform_rect(&bounds, &transform);
		fz_irect bbox;
		fz_round_rect(&bbox, &bounds);
		fz_pixmap *pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), &bbox);
		fz_clear_pixmap_with_value(ctx, pix, 0xff);
		fz_device *dev = fz_new_draw_device(ctx, pix);
		fz_run_page(ctx, page, dev, &transform, NULL);
		fz_drop_device(ctx,dev);
		cv::Mat input;
		imgProcessor::pixmap2mat(&pix, input);
		imageList.push_back(input.clone());
		fz_drop_pixmap(ctx, pix);
		fz_drop_page(ctx, page);
	}
	fz_drop_document(ctx,doc);
	fz_drop_context(ctx);
	return true;
}
//////////////////////////////////////////////////////////////