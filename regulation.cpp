#include "regulation.h"
#include <QDebug>

//CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//
#include <QList>
#include <QDir>
//CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//

Regulation::Regulation(): mirCtrl(){                                        //Creation of the regulation object
    qDebug()<<"Creation of Regulation object";

    ptsNumb = 100;                                                          //Cutting the circle into ptsNumb portions
    flag = 1;                                                               //Used for the step response
    vectorLength = 100;                                                     //100 for 180°, 68 for 122.4° and 32 for 57.6°
    d_pl = 17.02;                                                           //Distance between the center of the circle and the laser

    //const float kp =0;
    //const flaot ki =0;                                                    //Creation of the regulator parameters
    //const flaot kd =0;
}

bool Regulation::Initialisation(){
    bool properConnection = mirCtrl.Initialisation();                        //initialisation of the mirror
    return properConnection;
}


void Regulation::Figure (int type_recieved, float r, int x_desired, int y_desired){

    type = type_recieved, radius = r; desired_x = x_desired; desired_y = y_desired;
    d_pc = radius - d_pl;                                                             //Distance between the center of the circle and the particle

    if(type==1 || type==2){
        qDebug() << "Creation of the figure";
        float stepAngle = (float)M_PI*2 / ptsNumb;
        for( int i = 0; i < ptsNumb; i++ ){
            figure_x[i] = radius*cos( i * stepAngle );
            figure_y[i] = radius*sin( i * stepAngle );
        }
    }
    else{
        qDebug()<<"Use of the point actuation. No figure creation";
    }
}

void Regulation::Regulator (float particle_x, float particle_y){

        //PID REGULATION    //use of particule_x, particule_y, desired_x and desired_y
        //PID REGULATION    //gives the new center of the figure ==> float x and float y
        //PID REGULATION

        if (flag==1){   //Modification of the position of the laser at each image

            //For velocity tests and step repsonses, the center/point is fixed by the following rules
            x = particle_x;
            y = particle_y - d_pc;

            //For the step response, the laser should stop when it reaches the objective
//            if (y <= desired_y){
//                flag = 0;
//            }

            if (type==0){  //Point actuation
                mirCtrl.ChangeMirrorPosition(x, y);
            }

            else if (type==1){ //Circle actuation
                for(int j=0; j<ptsNumb; j++){
                    x_vector[j] = figure_x[j] + x;
                    y_vector[j] = figure_y[j] + y;
                }
                mirCtrl.ChangeMirrorStream (x_vector, y_vector);
            }

            else {              //Arc circle actuation
                                //Additional degree of freedom: orientation. Required more computations

//                //Calculation of objective angle
//                float objectiveAngle = atan((desired_y - particle_y) / (desired_x - particle_x));
//                //Calculation of the middleAngle, which corresponds to the angle at the middle of the arc figure
//                middleAngle = objectiveAngle + M_PI;
//                if (middleAngle >= 2*M_PI){
//                  middleAngle = middleAngle - 2*M_PI;
//                }
//                else if (middleAngle < 0){
//                  middleAngle = middleAngle + 2*M_PI;
//                }

                //for the velocity tests:
                middleAngle = (float)M_PI / 2;

                //Calculation of the index of the middleAngle within x_vector and y_vector
                int middleAngleIndex = middleAngle / ((float)M_PI*2 / ptsNumb) +0.5;

                //Construction of the arc circle
                int index = 0;
                int i = 0;
                int circleIndex;

                for(i; i <= (vectorLength/4); i++){
                    circleIndex = middleAngleIndex+index;
                    if (circleIndex>=vectorLength){
                        circleIndex = circleIndex - vectorLength;
                    }
                    else if (circleIndex < 0){
                        circleIndex = circleIndex + vectorLength;
                    }
                    x_vector[i] = figure_x[circleIndex] + x;
                    y_vector[i] = figure_y[circleIndex] + y;
                    index++;
                }

                index = index - 2;

                for(i; i <= (vectorLength*0.75); i++){
                    circleIndex = middleAngleIndex+index;
                    if (circleIndex>=vectorLength){
                        circleIndex = circleIndex - vectorLength;
                    }
                    else if (circleIndex < 0){
                        circleIndex = circleIndex + vectorLength;
                    }
                    x_vector[i] = figure_x[circleIndex] + x;
                    y_vector[i] = figure_y[circleIndex] + y;
                    index--;
                }

                index = index + 2;

                for(i; i < vectorLength; i++){
                    circleIndex = middleAngleIndex+index;
                    if (circleIndex>=vectorLength){
                        circleIndex = circleIndex - vectorLength;
                    }
                    else if (circleIndex < 0){
                        circleIndex = circleIndex + vectorLength;
                    }
                    x_vector[i] = figure_x[circleIndex] + x;
                    y_vector[i] = figure_y[circleIndex] + y;
                    index++;
                }

                mirCtrl.ChangeMirrorStream (x_vector, y_vector);
            }
        }

        else{   //The laser is fixed within the objective position
            qDebug() << "Objective reached";
        }

//        //CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//
//        QString currentData(QString::number(x_vector[j])                  //x_particle
//                            +","
//                            +QString::number(y_vector[j])                 //y_particle
//                            +",");
//        dataToSave.append(currentData);
//        //CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//

}


void Regulation::closeRegulation (){
    qDebug()<<"Closing the mirror";

//    //CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//
//    QString filename = "circle_reg.txt";
//    QFile file (filename);
//    file.open(QIODevice::WriteOnly);
//    QTextStream out(&file);
//    out << dataToSave;
//    file.close();
//    //CODE TO DELETE - USED TO VERIFY THE FUNCTIONING//

    mirCtrl.Closing();                                                  //Close the mirror
}