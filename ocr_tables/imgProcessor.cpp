#pragma once 

#include "imgProcessor.h"



bool imgProcessor::thresholdImg (cv::Mat& input, cv::Mat& output, double k, double dR)
{
	 if ((input.rows<=0) || (input.cols<=0)) 
	 {
		cerr << "*** ERROR: Invalid input Image " << endl;
		return false;
	 }
    
	int win = (int) (2.0 * input.rows-1)/3;
	win = std::min(win, input.cols-1);

    // Threshold
    output = cv::Mat(input.rows, input.cols, CV_8U);
    NiblackSauvolaWolfJolion (input, output, SAUVOLA, win, win, k, dR);
	output = 255*output;
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////
l_int32  imgProcessor::DoPageSegmentation(PIX *pixs, segmentationBlocks& blocks)
{
	l_int32      zero;
	BOXA        *boxatm, *boxahm;
	PIX         *pixr;   /* image reduced to 150 ppi */
	PIX         *pixhs;  /* image of halftone seed, 150 ppi */
	PIX         *pixm;   /* image of mask of components, 150 ppi */
	PIX         *pixhm1; /* image of halftone mask, 150 ppi */
	PIX         *pixhm2; /* image of halftone mask, 300 ppi */
	PIX         *pixht;  /* image of halftone components, 150 ppi */
	PIX         *pixnht; /* image without halftone components, 150 ppi */
	PIX         *pixi;   /* inverted image, 150 ppi */
	PIX         *pixvws; /* image of vertical whitespace, 150 ppi */
	PIX         *pixtm1; /* image of closed textlines, 150 ppi */
	PIX         *pixtm2; /* image of refined text line mask, 150 ppi */
	PIX         *pixtm3; /* image of refined text line mask, 300 ppi */
	PIX         *pixtb1; /* image of text block mask, 150 ppi */
	PIX         *pixtb2; /* image of text block mask, 300 ppi */
	PIX         *pixnon; /* image of non-text or halftone, 150 ppi */
	PIX         *pixt1, *pixt2, *pixt3;
	PIXCMAP     *cmap;
	PTAA        *ptaa;
	l_int32      ht_flag = 0;
	l_int32      ws_flag = 0;
	l_int32      text_flag = 0;
	l_int32      block_flag = 0;


	cv::Size defSize(pixs->w, pixs->h);
    pixr = pixReduceRankBinaryCascade(pixs, 1, 0, 0, 0);

    /* Get seed for halftone parts */
    pixt1 = pixReduceRankBinaryCascade(pixr, 4, 4, 3, 0);
    pixt2 = pixOpenBrick(NULL, pixt1, 5, 5);
    pixhs = pixExpandBinaryPower2(pixt2, 8);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);

    /* Get mask for connected regions */
    pixm = pixCloseSafeBrick(NULL, pixr, 4, 4);

    /* Fill seed into mask to get halftone mask */
    pixhm1 = pixSeedfillBinary(NULL, pixhs, pixm, 4);
    pixhm2 = pixExpandBinaryPower2(pixhm1, 2);

    /* Extract halftone stuff */
    pixht = pixAnd(NULL, pixhm1, pixr);

	/* Extract non-halftone stuff */
    pixnht = pixXor(NULL, pixht, pixr);
    pixZero(pixht, &zero);

	/* Get bit-inverted image */
    pixi = pixInvert(NULL, pixnht);

        /* The whitespace mask will break textlines where there
         * is a large amount of white space below or above.
         * We can prevent this by identifying regions of the
         * inverted image that have large horizontal (bigger than
         * the separation between columns) and significant
         * vertical extent (bigger than the separation between
         * textlines), and subtracting this from the whitespace mask. */
    pixt1 = pixMorphCompSequence(pixi, "o80.60", 0);
    pixt2 = pixSubtract(NULL, pixi, pixt1);
    pixDestroy(&pixt1);

    /* Identify vertical whitespace by opening inverted image */
    pixt3 = pixOpenBrick(NULL, pixt2, 5, 1);  /* removes thin vertical lines */
    pixvws = pixOpenBrick(NULL, pixt3, 1, 200);  /* gets long vertical lines */
	pix2mat(&pixvws, blocks.vert);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

    /* Get proto (early processed) text line mask. */
    /* First close the characters and words in the textlines */
    pixtm1 = pixCloseSafeBrick(NULL, pixnht, 30, 1);
 

	/* Next open back up the vertical whitespace corridors */
    pixtm2 = pixSubtract(NULL, pixtm1, pixvws);

    /* Do a small opening to remove noise */
    pixOpenBrick(pixtm2, pixtm2, 3, 3);
    pixtm3 = pixExpandBinaryPower2(pixtm2, 2);

        /* Join pixels vertically to make text block mask */
    pixtb1 = pixMorphSequence(pixtm2, "c1.10 + o4.1", 0);

        /* Solidify the textblock mask and remove noise:
         *  (1) For each c.c., close the blocks and dilate slightly
         *      to form a solid mask.
         *  (2) Small horizontal closing between components
         *  (3) Open the white space between columns, again
         *  (4) Remove small components */
    pixt1 = pixMorphSequenceByComponent(pixtb1, "c30.30 + d3.3", 8, 0, 0, NULL);
    pixCloseSafeBrick(pixt1, pixt1, 10, 1);
    pixt2 = pixSubtract(NULL, pixt1, pixvws);
    pixt3 = pixSelectBySize(pixt2, 25, 5, 8, L_SELECT_IF_BOTH,
                            L_SELECT_IF_GTE, NULL);
	pix2mat(&pixt3, blocks.text);
    pixtb2 = pixExpandBinaryPower2(pixt3, 2);
    pixDestroy(&pixt1);
    pixDestroy(&pixt2);
    pixDestroy(&pixt3);

        /* Identify the outlines of each textblock */
    ptaa = pixGetOuterBordersPtaa(pixtb2);
    pixt1 = pixRenderRandomCmapPtaa(pixtb2, ptaa, 1, 8, 1);
    cmap = pixGetColormap(pixt1);
    pixcmapResetColor(cmap, 0, 130, 130, 130);  /* set interior to gray */
     ptaaDestroy(&ptaa);
    pixDestroy(&pixt1);

        /* Fill line mask (as seed) into the original */
    pixt1 = pixSeedfillBinary(NULL, pixtm3, pixs, 8);
    pixOr(pixtm3, pixtm3, pixt1);
    pixDestroy(&pixt1);
 
        /* Fill halftone mask (as seed) into the original */
    pixt1 = pixSeedfillBinary(NULL, pixhm2, pixs, 8);
    pixOr(pixhm2, pixhm2, pixt1);
    pixDestroy(&pixt1);
	pix2mat(&pixhm2, blocks.figures);
  
        /* Find objects that are neither text nor halftones */
    pixt1 = pixSubtract(NULL, pixs, pixtm3);  /* remove text pixels */
    pixnon = pixSubtract(NULL, pixt1, pixhm2);  /* remove halftone pixels */
     pixDestroy(&pixt1);
	 pix2mat(&pixnon, blocks.other);

        /* Write out b.b. for text line mask and halftone mask components */
    boxatm = pixConnComp(pixtm3, NULL, 4);
    boxahm = pixConnComp(pixhm2, NULL, 8);

    //pixDestroy(&pixt1);
    //pixaDestroy(&pixa);

        /* clean up to test with valgrind */
    pixDestroy(&pixr);
    pixDestroy(&pixhs);
    pixDestroy(&pixm);
    pixDestroy(&pixhm1);
    pixDestroy(&pixhm2);
    pixDestroy(&pixht);
    pixDestroy(&pixnht);
    pixDestroy(&pixi);
    pixDestroy(&pixvws);
    pixDestroy(&pixtm1);
    pixDestroy(&pixtm2);
    pixDestroy(&pixtm3);
    pixDestroy(&pixtb1);
    pixDestroy(&pixtb2);
    pixDestroy(&pixnon);
    boxaDestroy(&boxatm);
    boxaDestroy(&boxahm);


	blocks.resize(defSize);
	//blocks.resize(blocks.text.size());
	blocks.invertColors();

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
double  imgProcessor::calcLocalStats (Mat &im, Mat &map_m, Mat &map_s, int winx, int winy)
{
	   Mat im_sum, im_sum_sq;
    cv::integral(im,im_sum,im_sum_sq,CV_64F);

	double m,s,max_s,sum,sum_sq;	
	int wxh	= winx/2;
	int wyh	= winy/2;
	int x_firstth= wxh;
	int y_lastth = im.rows-wyh-1;
	int y_firstth= wyh;
	double winarea = winx*winy;

	max_s = 0;
	for	(int j = y_firstth ; j<=y_lastth; j++){   
		sum = sum_sq = 0;

        sum = im_sum.at<double>(j-wyh+winy,winx) - im_sum.at<double>(j-wyh,winx) - im_sum.at<double>(j-wyh+winy,0) + im_sum.at<double>(j-wyh,0);
        sum_sq = im_sum_sq.at<double>(j-wyh+winy,winx) - im_sum_sq.at<double>(j-wyh,winx) - im_sum_sq.at<double>(j-wyh+winy,0) + im_sum_sq.at<double>(j-wyh,0);

		m  = sum / winarea;
		s  = sqrt ((sum_sq - m*sum)/winarea);
		if (s > max_s) max_s = s;

		map_m.fset(x_firstth, j, m);
		map_s.fset(x_firstth, j, s);

		// Shift the window, add and remove	new/old values to the histogram
		for	(int i=1 ; i <= im.cols-winx; i++) {

			// Remove the left old column and add the right new column
			sum -= im_sum.at<double>(j-wyh+winy,i) - im_sum.at<double>(j-wyh,i) - im_sum.at<double>(j-wyh+winy,i-1) + im_sum.at<double>(j-wyh,i-1);
			sum += im_sum.at<double>(j-wyh+winy,i+winx) - im_sum.at<double>(j-wyh,i+winx) - im_sum.at<double>(j-wyh+winy,i+winx-1) + im_sum.at<double>(j-wyh,i+winx-1);

			sum_sq -= im_sum_sq.at<double>(j-wyh+winy,i) - im_sum_sq.at<double>(j-wyh,i) - im_sum_sq.at<double>(j-wyh+winy,i-1) + im_sum_sq.at<double>(j-wyh,i-1);
			sum_sq += im_sum_sq.at<double>(j-wyh+winy,i+winx) - im_sum_sq.at<double>(j-wyh,i+winx) - im_sum_sq.at<double>(j-wyh+winy,i+winx-1) + im_sum_sq.at<double>(j-wyh,i+winx-1);

			m  = sum / winarea;
			s  = sqrt ((sum_sq - m*sum)/winarea);
			if (s > max_s) max_s = s;

			map_m.fset(i+wxh, j, m);
			map_s.fset(i+wxh, j, s);
		}
	}

	return max_s;
}
//////////////////////////////////////////////////////////////////////////////////////////
void  imgProcessor::NiblackSauvolaWolfJolion (Mat im, Mat output, NiblackVersion version, int winx, int winy, double k, double dR)
{
	double m, s, max_s;
	double th=0;
	double min_I, max_I;
	int wxh	= winx/2;
	int wyh	= winy/2;
	int x_firstth= wxh;
	int x_lastth = im.cols-wxh-1;
	int y_lastth = im.rows-wyh-1;
	int y_firstth= wyh;
	//int mx, my;

	// Create local statistics and store them in a double matrices
	Mat map_m = Mat::zeros (im.rows, im.cols, CV_32F);
	Mat map_s = Mat::zeros (im.rows, im.cols, CV_32F);
	max_s = calcLocalStats (im, map_m, map_s, winx, winy);
	
	minMaxLoc(im, &min_I, &max_I);
			
	Mat thsurf (im.rows, im.cols, CV_32F);
			
	// Create the threshold surface, including border processing
	// ----------------------------------------------------

	for	(int j = y_firstth ; j<=y_lastth; j++) {

		// NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
		for	(int i=0 ; i <= im.cols-winx; i++) {

			m  = map_m.fget(i+wxh, j);
    		s  = map_s.fget(i+wxh, j);

    		// Calculate the threshold
    		switch (version) {

    			case NIBLACK:
    				th = m + k*s;
    				break;

    			case SAUVOLA:
	    			th = m * (1 + k*(s/dR-1));
	    			break;

    			case WOLFJOLION:
    				th = m + k * (s/max_s-1) * (m-min_I);
    				break;
    				
    			default:
    				cerr << "Unknown threshold type in ImageThresholder::surfaceNiblackImproved()\n";
    				exit (1);
    		}
    		
    		thsurf.fset(i+wxh,j,th);

    		if (i==0) {
        		// LEFT BORDER
        		for (int i=0; i<=x_firstth; ++i)
                	thsurf.fset(i,j,th);

        		// LEFT-UPPER CORNER
        		if (j==y_firstth)
        			for (int u=0; u<y_firstth; ++u)
        			for (int i=0; i<=x_firstth; ++i)
        				thsurf.fset(i,u,th);

        		// LEFT-LOWER CORNER
        		if (j==y_lastth)
        			for (int u=y_lastth+1; u<im.rows; ++u)
        			for (int i=0; i<=x_firstth; ++i)
        				thsurf.fset(i,u,th);
    		}

			// UPPER BORDER
			if (j==y_firstth)
				for (int u=0; u<y_firstth; ++u)
					thsurf.fset(i+wxh,u,th);

			// LOWER BORDER
			if (j==y_lastth)
				for (int u=y_lastth+1; u<im.rows; ++u)
					thsurf.fset(i+wxh,u,th);
		}

		// RIGHT BORDER
		for (int i=x_lastth; i<im.cols; ++i)
        	thsurf.fset(i,j,th);

  		// RIGHT-UPPER CORNER
		if (j==y_firstth)
			for (int u=0; u<y_firstth; ++u)
			for (int i=x_lastth; i<im.cols; ++i)
				thsurf.fset(i,u,th);

		// RIGHT-LOWER CORNER
		if (j==y_lastth)
			for (int u=y_lastth+1; u<im.rows; ++u)
			for (int i=x_lastth; i<im.cols; ++i)
				thsurf.fset(i,u,th);
	}
	//cerr << "surface created" << endl;
	
	
	for	(int y=0; y<im.rows; ++y) 
	for	(int x=0; x<im.cols; ++x) 
	{
    	if (im.uget(x,y) >= thsurf.fget(x,y))
    	{
    		output.uset(x,y,255);
    	}
    	else
    	{
    	    output.uset(x,y,0);
    	}
    }
}
//////////////////////////////////////////////////////////////////////////////////////////
bool imgProcessor::mat2pix (cv::Mat& mat, Pix** px)
{
	if ((mat.type() !=CV_8UC1) || (mat.empty())) return false;

	*px = pixCreate(mat.cols, mat.rows, 8);
	uchar* data = mat.data;
	
	for(int i=0; i<mat.rows; i++)
	{
		unsigned idx = i*mat.cols;
		for(int j=0; j<mat.cols; j++) 
		{
			pixSetPixel(*px, j,i, (l_uint32)data[idx+j]) ;
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool imgProcessor::mat2pixBinary (cv::Mat& mat, Pix** px)
{
	if ((mat.type() !=CV_8UC1) || (mat.empty())) return false;

	*px = pixCreate(mat.cols, mat.rows, 1);
	uchar* data = mat.data;
	
	for(int i=0; i<mat.rows; i++)
	{
		unsigned idx = i*mat.cols;
		for(int j=0; j<mat.cols; j++) 
		{
			pixSetPixel(*px, j,i, 1-(l_uint32)(data[idx+j]/255)) ;
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////
bool imgProcessor::pix2mat (Pix** px, cv::Mat& mat)
{
	if (	((*px)->d != 8 && (*px)->d != 1) 
		||	((*px)->w < 1)) return false;
	
	mat = cv::Mat((*px)->h,(*px)->w, CV_8UC1);
	uchar* data = mat.data;

	if ((*px)->d == 8)
	{
		for(int i=0; i<mat.rows; i++)
		{
			unsigned idx = i*mat.cols;
			for(int j=0; j<mat.cols; j++) 
			{
				l_uint32 val;
				pixGetPixel(*px, j, i, &val);
				data[idx+j] = (uchar) val;
			}
		}
	}
	else
	{
		for(int i=0; i<mat.rows; i++)
		{
			unsigned idx = i*mat.cols;
			for(int j=0; j<mat.cols; j++) 
			{
				l_uint32 val;
				pixGetPixel(*px, j, i, &val);
				data[idx+j] = (uchar) (255-val*255);
			}
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
bool imgProcessor::pixmap2mat (fz_pixmap** fzpxmap, cv::Mat& mat)
{
	if ( (*fzpxmap)->w <1) return false;
	mat = cv::Mat((*fzpxmap)->h, (*fzpxmap)->w, CV_8UC1);
	uchar* data = mat.data;

	for (unsigned i=0;i< (*fzpxmap)->h;i++)
	{
		unsigned idxMat = i*mat.cols;
		unsigned idxPixmap = i*4*mat.cols;
		for (unsigned j=0;j<(*fzpxmap)->w;j++)
		{
			data[idxMat + j] = ((*fzpxmap)->samples[idxPixmap + 4*j ] + (*fzpxmap)->samples[idxPixmap + 4*j + 1] + (*fzpxmap)->samples[idxPixmap + 4*j + 2])/3;
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
void imgProcessor::prepareAll(cv::Mat& input, cv::Mat& thres, segmentationBlocks& blocks)
{
	std::cout << "Thresholding Image...";
	
	
	cv::Mat tr,tr2;
	cv::erode(input,tr2,cv::Mat(), cv::Point(-1,-1), 1);
	imgProcessor::thresholdImg(tr2, thres);
	cv::erode(thres,tr,cv::Mat(), cv::Point(-1,-1), 3);
	std::cout << "Done!\n";
	Pix* px = NULL;
	imgProcessor::mat2pixBinary(tr, &px);
	std::cout << "Segmenting Page...";
	imgProcessor::DoPageSegmentation(px, blocks);
	std::cout << "Done!\n";
	pixDestroy(&px);
	imgProcessor::thresholdImg(input, thres);
}
/////////////////////////////////////////////////////////////////////////////////////////
void imgProcessor::prepareAll(fz_pixmap** fzpxmap, cv::Mat& thres, segmentationBlocks& blocks)
{
	cv::Mat input;
	pixmap2mat(fzpxmap, input);
	prepareAll(input, thres, blocks);
}
/////////////////////////////////////////////////////////////////////////////////////////
void imgProcessor::prepareAll(Pix** px, cv::Mat& thres, segmentationBlocks& blocks)
{
	cv::Mat input;
	pix2mat(px, input);
	prepareAll(input, thres, blocks);
}
/////////////////////////////////////////////////////////////////////////////////////////
void imgProcessor::getTextImage(cv::Mat& input, segmentationBlocks& blk, cv::Mat& output)
{
	blk.text.setTo(0, blk.figures==255);
	blk.text.setTo(0, blk.vert==255);
	cv::dilate(blk.text,blk.text,cv::Mat(),cv::Point(-1,-1),5);
	cv::dilate(blk.figures,blk.figures,cv::Mat(),cv::Point(-1,-1),10);
	output=input.clone();
	output.setTo(255, blk.figures==255);
	output.setTo(255, blk.text==0);
	//output.setTo(255, blk.other==255);


}
/////////////////////////////////////////////////////////////////////////////////////////
void imgProcessor::reorderImage(cv::Mat& input, segmentationBlocks& blk, cv::Mat& output)
{
	cv::Mat mask (blk.text>100);

	cv::Mat vertSpaces;
	cv::reduce(mask, vertSpaces, 0, CV_REDUCE_SUM, CV_32FC1);
	float* data = (float*)vertSpaces.data;

	std::vector<int> emptyCols, trueEmptyCols;

	emptyCols.push_back(0);
	for (unsigned i=1;i<vertSpaces.cols-1;i++)
	{
		if (data[i]==0 && data[i+1]!=0) emptyCols.push_back(i);
	}
	emptyCols.push_back(vertSpaces.cols-1);

	for (unsigned i=0;i<emptyCols.size()-1;i++)
	{
		if (emptyCols[i]!=emptyCols[i+1])
		{
			cv::Rect rct(emptyCols[i],0,emptyCols[i+1]-emptyCols[i],1);
			if (cv::sum(vertSpaces(rct))[0]!=0)
			{
				if (trueEmptyCols.empty()) trueEmptyCols.push_back(emptyCols[i]);
				trueEmptyCols.push_back(emptyCols[i+1]);
			}
		}
	}

	if (trueEmptyCols.size()>5) //too many columns, probably full page matrix
	{
		output = input.clone();
		return;
	}

	if (trueEmptyCols.size()>2) //if columns are uniform then possibly tey are text columns. else either matrix or simply formatted text
	{
		float avg_width=0,var_width=0;
		for (unsigned i=0;i<trueEmptyCols.size()-1;i++)
		{
			avg_width+=trueEmptyCols[i+1]-trueEmptyCols[i];
		}
		avg_width/=trueEmptyCols.size()-1;
		for (unsigned i=0;i<trueEmptyCols.size()-1;i++)
		{
			var_width+= std::pow((trueEmptyCols[i+1]-trueEmptyCols[i] - avg_width),2);
		}
		var_width/=trueEmptyCols.size()-1;
		var_width=std::sqrt(var_width);

		if (var_width<100) //probably multi column text
		{
			output = cv::Mat((input.rows+10)*(trueEmptyCols.size()-1),input.cols, CV_8UC1);
			output.setTo(255);
			for (unsigned i=0;i<trueEmptyCols.size()-1;i++)
			{
				cv::Rect rt(0, i*(input.rows+10), trueEmptyCols[i+1]-trueEmptyCols[i], input.rows);
				cv::Rect rt2(trueEmptyCols[i], 0, trueEmptyCols[i+1]-trueEmptyCols[i], input.rows);
				cv::Mat tmp2 = input(rt2);
				tmp2.copyTo(output(rt));
			}
			return;
		}
		output = input.clone();
		return;
	}

	//if non of the above works, check for subareas between empty rows


	cv::Mat horSpaces;
	cv::reduce(mask, horSpaces, 1, CV_REDUCE_SUM, CV_32FC1);
	data = (float*)horSpaces.data;

	std::vector<int> emptyRows, trueEmptyRows;

	emptyRows.push_back(0);
	for (unsigned i=1;i<horSpaces.rows-1;i++)
	{
		if (data[i]==0 && data[i+1]!=0) emptyRows.push_back(i);
	}
	emptyRows.push_back(horSpaces.rows-1);

	for (unsigned i=0;i<emptyRows.size()-1;i++)
	{
		if (emptyRows[i]!=emptyRows[i+1])
		{
			cv::Rect rct(0, emptyRows[i],1,emptyRows[i+1]-emptyRows[i]);
			if (cv::sum(horSpaces(rct))[0]!=0)
			{
				if (trueEmptyRows.empty()) trueEmptyRows.push_back(emptyRows[i]);
				trueEmptyRows.push_back(emptyRows[i+1]);
			}
		}
	}

	if (trueEmptyRows.size()>6) //too many rows, probably full page matrix
	{
		output = input.clone();
		return;
	}

	if (trueEmptyRows.size()<2) // no page segmentation available. single column text
	{
		output = input.clone();
		return;
	}

	//search for columned text between empty lines
	std::vector <cv::Mat> outputParts;
	for (unsigned s=0;s<trueEmptyRows.size()-1;s++)
	{
		bool hasColumns=false;
		cv::Rect rcts(0,trueEmptyRows[s], input.cols, trueEmptyRows[s+1]-trueEmptyRows[s]);
		cv::Mat part = mask(rcts);
		std::vector<int> localEmptyCols, localTrueEmptyCols;
		if ( rcts.height > (float)input.rows/10)
		{
			cv::Mat localVertSpaces;
			cv::reduce(part, localVertSpaces, 0, CV_REDUCE_SUM, CV_32FC1);
			data = (float*)localVertSpaces.data;
				

			localEmptyCols.push_back(0);
			for (unsigned i=1;i<localVertSpaces.cols-1;i++)
			{
				if (data[i]==0 && data[i+1]!=0) localEmptyCols.push_back(i);
			}
			localEmptyCols.push_back(localVertSpaces.cols-1);

			for (unsigned i=0;i<localEmptyCols.size()-1;i++)
			{
				if (localEmptyCols[i]!=localEmptyCols[i+1])
				{
					cv::Rect rct(localEmptyCols[i],0,localEmptyCols[i+1]-localEmptyCols[i],1);
					if (cv::sum(localVertSpaces(rct))[0]!=0)
					{
						if (localTrueEmptyCols.empty()) localTrueEmptyCols.push_back(localEmptyCols[i]);
						localTrueEmptyCols.push_back(localEmptyCols[i+1]);
					}
				}
			}

			if (localTrueEmptyCols.size()>2 && localTrueEmptyCols.size()<5)
			{
				float avg_width=0,var_width=0;
				for (unsigned i=0;i<localTrueEmptyCols.size()-1;i++)
				{
					avg_width+=localTrueEmptyCols[i+1]-localTrueEmptyCols[i];
				}
				avg_width/=localTrueEmptyCols.size()-1;
				for (unsigned i=0;i<localTrueEmptyCols.size()-1;i++)
				{
					var_width+= std::pow((localTrueEmptyCols[i+1]-localTrueEmptyCols[i] - avg_width),2);
				}
				var_width/=localTrueEmptyCols.size()-1;
				var_width=std::sqrt(var_width);

				if (var_width<100)
				{
					hasColumns=true;
				}
			}
		}

		if (!hasColumns)
		{
			outputParts.push_back(input(rcts).clone());
		}
		else
		{
			cv::Mat prt((part.rows+10)*(localTrueEmptyCols.size()-1),input.cols, CV_8UC1);
			prt.setTo(255);
			for (unsigned i=0;i<localTrueEmptyCols.size()-1;i++)
			{
				cv::Rect rt(0, i*(part.rows+10), localTrueEmptyCols[i+1]-localTrueEmptyCols[i], part.rows);
				cv::Rect rt2(localTrueEmptyCols[i], 0, localTrueEmptyCols[i+1]-localTrueEmptyCols[i], part.rows);
				cv::Mat tmp2 =(input(rcts))(rt2);
				tmp2.copyTo(prt(rt));
			}
			outputParts.push_back(prt.clone());
		}
	}
	output = outputParts[0].clone();
	for (unsigned i=1;i<outputParts.size();i++)
	{
		cv::vconcat(output,outputParts[i],output);
	}

}
