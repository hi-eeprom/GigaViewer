#include "ellipsedetection.h"


using namespace cv;

EllipseDetection::EllipseDetection(int thresh) : threshold(thresh),activated(false),shouldTrack(false){

}

void EllipseDetection::ChangeSettings(QMap<QString,QVariant> settings) {
    targetX=settings["targetX"].toInt();
    targetY=settings["targetY"].toInt();
    threshold=settings["threshold"].toInt();
    if ((!activated)&&(settings["activated"].toBool())) {
       // qDebug()<<"Should initialise";
    }
    if (activated&&(!settings["activated"].toBool())) {
       // qDebug()<<"Should write to disk";
    }

    activated=settings["activated"].toBool();
    shouldTrack=settings["shouldTrack"].toBool();
//    qDebug()<<"threshold="<<threshold;
}

bool EllipseDetection::processImage(ImagePacket& currIm) {
    if (activated) {


        if (currIm.pixFormat=="RGB8") {
            cv::Mat grayIm;
            cv::cvtColor(currIm.image,grayIm,CV_RGB2GRAY);
            currIm.image=grayIm.clone();
            currIm.pixFormat="MONO8";
        }
        if (currIm.pixFormat=="MONO8") {
            int newT=threshold*255/100.0;  //change here if it needs to work for 16-bit

            int apSize=3;
            // Find edge pixels
            Mat I=currIm.image.clone();
            Mat edges;
//            Mat sobx,soby,sobang,sobmag,edges;
//            Sobel( I, sobx,CV_32F,1, 0, apSize );
//            Sobel( I, soby,CV_32F, 0, 1, apSize );
//            cartToPolar(sobx,soby,sobmag,sobang);
            Canny( I, edges,0.5*newT,newT,apSize,true);

            Mat labs; // = Mat::zeros( edges.size(), CV_32S );  // should be able to store all contours in a different label => 8U not suff
            Mat stats;// = Mat::zeros( edges.size(), CV_32S );  // should be able to store all contours in a different label => 8U not suff
            Mat centres;
            int nrLabs=connectedComponentsWithStats(edges,labs,stats,centres);

            //            qDebug()<<"Amount of labels: "<<nrLabs;
            if (nrLabs>1) {
                // now do single loop over labs to create a vector of vector of points where each subvector contains the coordinates
                // of that label. Also make a separate vector to contain amount of edge points for each label.
                // Easier to eliminate too small or too large labels before full loop.
                int32_t* pixPointer;
                std::vector<std::vector<Point> > labCont(nrLabs);// Vecteur de listes de point
                std::vector<int> counter(nrLabs);//Vecteur d'entiers

                for (int i=0;i<labs.rows;i++) {
                    pixPointer=labs.ptr<int32_t>(i);
                    for (int j=0;j<labs.cols;j++) {
                        int32_t label=pixPointer[j];
                        if (label==0) continue;

                        labCont[label-1].push_back(Point(j,i));
                        if (labCont[label-1].size()==0) {
                            counter[label-1]=1;
                        } else {
                            counter[label-1]=counter[label-1]+1;
                        }

                    }
                }

//                vector<Mat> accum;
//                int solCounter=0;
//                int resColumns=0;
                int minContour= 20; //Nombre minimum de points de contours
                int maxSize=max(I.rows,I.cols);// ?
                float minDiam = targetX/100.0*maxSize; // Longueur du petit axe de l'ellipse
                float maxDiam = targetY/100.0*maxSize; // Longueur du grand axe de l'ellipse
                std::vector<RotatedRect> foundEllipses; // Ellipse trouvé. Ellipse se trouve sous la forme d'un vecteur de type RoatedRect
// RotatedRect a trois paramètre : un axe hauteur, un axe longueur et un angle.
                for (int i=0;i<nrLabs;i++) // Pour tout les objets trouvés
                {
                    if (counter[i]<=minContour)// Si le nombre de point de contours de l'objet est inférieur au nombre minimum de points de contours
                    {
                        continue; // Nous passons au prochain objet
                    } else {
                        RotatedRect fEll=cv::fitEllipse(labCont[i]); // Dessinne les contours de l'objet ( les points sont reliés par une ligne continue) en forme d'ellpise
                        float aspRat=fEll.size.width/fEll.size.height;// Excentricité de l'ellipse
                        double minAxis=min(fEll.size.width,fEll.size.height)/2;// La longueur du petit axe de l'ellipse est le minimum entre la largeur et la hauteur
                        double maxAxis=max(fEll.size.width,fEll.size.height)/2;// La longueur du grand axe de l'ellipse est le maximum entre la largeur et la hauteur
                        Point Center=fEll.center;

//                        qDebug()<<"Min and max are: "<<minAxis<<" - "<<maxAxis;
                        if (aspRat>0.8 && aspRat < 1.2) // Si l'excentricité est comprise entre 0.8 et 1.2 ( donc une ellipse proche d'un cercle)
                        {
                            double Focal1x,Focal1y,Focal2x,Focal2y;// Création des 2 foyers de l'ellipse
                            double Cdouble=sqrt(pow(maxAxis,2)-pow(minAxis,2));//Distance du centre de l'ellipse à l'un des foyers

                            Focal1x=Center.x+cos(fEll.angle)*Cdouble;// Calcul de l'abscisse du 1er foyer
                            Focal1y=sin(fEll.angle)*Cdouble-Center.y;// Calcul de l'ordonnées du 1er foyer
                            Focal2x=cos(fEll.angle)*Cdouble-Center.x;// Calcul de l'abscisse du 2ème foyer
                            Focal2y=Center.y+sin(fEll.angle)*Cdouble;// Calcul de l'ordonnées du 2ème foyer
                            double maxDist=20;
                            double minValidationRatio=0.2;

                            if (minAxis>minDiam && maxAxis<maxDiam)// Si la longueur du petit axe est plus grand que ? et la longueur du grand axe est plus grand que ?
                            {
                                int NumberOfPointsGood=0;
                                for(int k=1;k<counter[i];k++)
                                {
                                    double xm=static_cast<double>(labCont[i][k].x);
                                    double ym=static_cast<double>(labCont[i][k].y);
                                    if ((sqrt(pow(xm-Focal1x,2)+pow(ym-Focal1y,2))+sqrt(pow(xm-Focal2x,2)+pow(ym-Focal2y,2))-2*maxDiam)<maxDist)
                                    {
                                        NumberOfPointsGood+=1;
                                    }
                                }
                                double ValidationRatio=NumberOfPointsGood/(2*3.1415*sqrt(minAxis*maxAxis));

                                if (ValidationRatio>minValidationRatio)

                                {
                                    qDebug()<<"ValRaio: "<<ValidationRatio;
                                    cv::ellipse(I,fEll,150,5); // ?
                                    foundEllipses.push_back(fEll); // ?

                                }
                            }
                        }
                    }

                }

                // now calculate mean equivalent diameter
                double accumMean=0;
                for (uint i=0;i<foundEllipses.size();i++) {
                    accumMean=accumMean+sqrt(foundEllipses[i].size.width*foundEllipses[i].size.height);
                }
                double myMean=accumMean/foundEllipses.size();
                //qDebug()<<"Mean: "<<myMean;
                char str[200];
                sprintf(str,"D=%.2f",myMean);
                putText(I, str, Point2f(20,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));



                currIm.image=I;
            } else {
                currIm.image=edges;
            }



//            cv::threshold ( currIm.image, processed, newT, 255, THRESH_BINARY_INV );

//            std::vector<std::vector<cv::Point> > contours;
//            cv::Mat hierarchy;
//            cv::findContours( processed, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

//            cv::Mat outImage=currIm.image.clone(); // use a clone to avoid storing on raw image which is stored.
//            cv::drawContours(outImage,contours,-1,255,2);

//            cv::Point newLoc(targetX*currIm.image.cols/100.0,targetY*currIm.image.rows/100.0);
//            cv::ellipse(outImage,newLoc,cv::Size(5,5),0,0,360,150,-1);

//            currIm.image=outImage;

            // now make string to save
            //QString outst(""
//            outst=""+
//            currIm.timeStamp+" "+threshold+" "+targetX+""
//            dataToSave.append(outst);


        }
    }
    return true;
}

