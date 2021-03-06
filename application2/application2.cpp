#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>

#include "../maxflow/graph.h"

using namespace std;
using namespace cv;

//classe contenant une image et un tableau pour stocker les pixels sélectionnés par l'utilisateur
struct Data{
	Mat Img;
	Mat preflot;
	int indicatif;
	int nbr;
	int tabx[4];
	int taby[4];

	Data(Mat I, Mat pf, int ii)
	{
		Img=I;
		preflot=pf;
		indicatif=ii;
		nbr=0;
	}
};

//teste si (i,j) appartient au quadrilatère défini par les tableaux tabx et taby
//e permet de dire si l'on considère l'intérieur (e=0) ou l'extérieur (e=1) du quadrilatère
bool appartient(int i, int j, int tabx[], int taby[], int e)
{
	bool b=true;
	bool ext=(e==1);
	int det;
	for (int k=0; k<4; k++)
	{
		det=(tabx[(k+1)%4]-tabx[k])*(j-taby[k])-(taby[(k+1)%4]-taby[k])*(i-tabx[k]);
		if (det<=0){b=false; return b^ext;}
	}
	return b^ext;
}

//initialisation à zéro de la matrice qui va contenir, pour chaque pixel (=chaque case) l'information comme quoi ce pixel a été sélectionné ou non dans les quadrilatères tracés par l'utilisateur
void pretraite(Mat& pf)
{
	int xx=pf.rows;
	int yy=pf.cols;
	for (int i=0; i<yy;i++)
	{
		for (int j=0; j<xx;j++)
		{
			pf.at<uchar>(j,i)=0;
		}
	}
}

//remplissage de la matrice pf précédente
//indicatif représente le numéro de l'image que l'on considère (1 ou 2)
void traite(Mat& pf, int tabx[], int taby[],int indicatif, int ext)
{
	int xx=pf.rows;
	int yy=pf.cols;
	for (int i=0; i<yy; i++)
	{
		for (int j=0; j<xx; j++)
		{
			if (appartient(i,j,tabx,taby,ext) && pf.at<uchar>(j,i)==0)
			{
				pf.at<uchar>(j,i)=indicatif;
			}
			else if (appartient(i,j,tabx,taby,ext))
			{
				pf.at<uchar>(j,i)=0; //un même pixel ne peut pas être sélectionné dans les deux quadrilatères
			}
		}
	}
}

//detection de la souris, et des clics sur les images
void CallBackFunc(int event, int x, int y, int flags, void *userdata)
{
	Data* D=(Data*)userdata;
	if(event==EVENT_LBUTTONDOWN && D->nbr<4)
	{
		circle(D->Img,Point(x,y),2,Scalar(0,-255*(D->indicatif-2),255*(D->indicatif-1)),2);
		D->tabx[D->nbr]=x;
		D->taby[D->nbr]=y;
		D->nbr++;
		if (D->indicatif==1)
		{
			imshow("I1",D->Img);
		}
		if (D->indicatif==2)
		{
			imshow("I2",D->Img);
		}
		if (D->nbr==4)
		{
			if (D->indicatif==1)
			{
				pretraite(D->preflot);
			}
			if (D->indicatif==1)
			{
				imshow("I1",D->Img);
			}
			if (D->indicatif==2)
			{
				imshow("I2",D->Img);
			}
			int ext;
			cout<<"Tapez 1 pour extérieur, 0 pour intérieur... ";
			cin>>ext;
			cout<<"Il faut recliquer sur une image et appuyer sur une touche pour passer à la suite"<<endl;
			traite(D->preflot,D->tabx,D->taby,D->indicatif,ext);
		}
	}
}


//fonction produisant le gradient de I, le gradient selon x et le gradient selon y
void gradient(Mat& I, Mat& out, Mat& outx, Mat& outy)
{
       	Mat NB;
        cvtColor(I, NB, CV_BGR2GRAY);
        int m = NB.rows, n = NB.cols;
        Mat Ix(m, n, CV_32F), Iy(m, n, CV_32F);
        for (int i = 0; i<m; i++) {
            for (int j = 0; j<n; j++){
               	float ix, iy;
                if (i == 0 || i == m - 1)
                   	iy = 0;
                else
                   	iy = (float(NB.at<uchar>(i + 1, j)) - float(NB.at<uchar>(i-1,j)))/2;
               	if (j == 0 || j == n - 1)
                   	ix = 0;
               	else
			ix=(float(NB.at<uchar>(i,j+1))-float(NB.at<uchar>(i,j-1)))/2;
		Ix.at<float>(i,j)=ix;
		Iy.at<float>(i,j)=iy;
     		out.at<float>(i, j) = sqrt(ix*ix + iy*iy);
     		}
  	}
	outx=Ix;
	outy=Iy;
}

//norme d'un vecteur (x,y)
double norme(float x, float y)
{
	return sqrt(x*x+y*y);
}

//distance entre deux Vec3b
double norme_dist(Vec3b src, Vec3b comp)
{
	int v0=src[0]-comp[0];
	int v1=src[1]-comp[1];
	int v2=src[2]-comp[2];
	int car=v0*v0+v1*v1+v2*v2;
	return (int)sqrt(car);
}

//distance entre A(x,y) et B(x,y)
double dist(Mat& A, Mat& B, int x, int y)
{
	return norme_dist(A.at<Vec3b>(x,y),B.at<Vec3b>(x,y));
}

//1re fonction de cout
double cost(Mat& A, Mat& B, int x1, int y1, int x2, int y2)
{
	return dist(A,B,x1,y1)+dist(A,B,x2,y2);
}

//deuxième fonction de cout, tenant compte du gradient
double cost2(Mat&A, Mat&B, int x1, int y1, int x2, int y2, Mat& GxA, Mat& GyA, Mat& GxB, Mat& GyB)
{
	Mat GA;
	Mat GB;
	if (x1==x2)
	{
		GA=GxA;
		GB=GxB;
	}
	else
	{
		GA=GyA;
		GB=GyB;
	}
	double grad=norme(GA.at<float>(x1,y1),0);
	grad=grad+norme(GA.at<float>(x2,y2),0);
	grad=grad+norme(GB.at<float>(x1,y1),0);
	grad=grad+norme(GB.at<float>(x2,y2),0);
	grad=pow(grad,0.26);
	double cc=cost(A,B,x1,y1,x2,y2);
	if (cc==0.0&&grad==0.0){return 0;}else{return cc/grad;}
}

int main(){

	Mat I1=imread("../../dauphin2.jpg");
	Mat I2=imread("../../lac_x.jpg");


	cout << "Images de depart" << endl;
	imshow("I1",I1);
	imshow("I2",I2);
	waitKey();
	
	const int DIFF_ROWS=I1.rows-I2.rows;
	const int DIFF_COLS=I1.cols-I2.cols;
	int largeur=max(I1.cols,I2.cols);
	int hauteur=max(I1.rows,I2.rows);
	
	//Creation d'images de meme taille
	Mat I1bis(hauteur,largeur,CV_8UC3);
	Mat I2bis(hauteur,largeur,CV_8UC3);

	for (int i=0; i<hauteur; i++)
	{
		for(int j=0; j<largeur; j++)
		{
			if (i<I1.rows&&j<I1.cols)
			{
				I1bis.at<Vec3b>(hauteur-i,j)=I1.at<Vec3b>(I1.rows-i,j);
			}
			else
			{
				I1bis.at<Vec3b>(hauteur-i,j)=Vec3b(0,0,0);
			}
			if (i<I2.rows&&j<I2.cols)
			{
				I2bis.at<Vec3b>(hauteur-i,j)=I2.at<Vec3b>(I2.rows-i,j);
			}
			else
			{
				I2bis.at<Vec3b>(hauteur-i,j)=Vec3b(0,0,0);
			}


		}
	}
	I1=I1bis;
	I2=I2bis;
	Mat Gx1(hauteur,largeur,CV_32F);
	Mat Gx2(hauteur,largeur,CV_32F);
	Mat Gy1(hauteur,largeur,CV_32F);
	Mat Gy2(hauteur,largeur,CV_32F);
	Mat G1(hauteur,largeur,CV_32F);
	Mat G2(hauteur,largeur,CV_32F);
	gradient(I1,G1,Gx1,Gy1);
	gradient(I2,G2,Gx2,Gy2);

	//Copie des images (pour le choix des points...)
	Mat I1copie(hauteur,largeur,CV_8UC3);
	Mat I2copie(hauteur,largeur,CV_8UC3);

	for (int i=0; i<hauteur; i++)
	{
		for(int j=0; j<largeur; j++)
		{
			I1copie.at<Vec3b>(i,j)=I1.at<Vec3b>(i,j);
			I2copie.at<Vec3b>(i,j)=I2.at<Vec3b>(i,j);
		}
	}

	//Redimensionnement
	imshow("I1",I1);
	imshow("I2",I2);

	//taille du graphe de flot
	int LARG=largeur-abs(DIFF_COLS);
	int HAUT=hauteur-abs(DIFF_ROWS);

	//Choix des points des images par l'utilisateur
	Mat preflot(hauteur,largeur,CV_8U);
	Data D1(I1copie,preflot,1);
	Data D2(I2copie,preflot,2);

	cout << "Choix des points de l'image 1, dans le sens horaire"<<endl;
	setMouseCallback("I1",CallBackFunc, &D1);
	waitKey();
	setMouseCallback("I1",NULL,NULL);

	cout << "Choix des points de l'image 2, dans le sens horaire"<<endl;
	setMouseCallback("I2",CallBackFunc, &D2);
	waitKey();
	setMouseCallback("I2",NULL,NULL);

	//dessin des quadrilatères selectionnés sur les images
	for (int i=0; i<largeur; i++)
	{
		for (int j=0; j<hauteur; j++)
		{
			if (preflot.at<uchar>(j,i)==1)
			{
				I1copie.at<Vec3b>(j,i)=Vec3b(0,255,0);
			}
			if (preflot.at<uchar>(j,i)==2)
			{
				I2copie.at<Vec3b>(j,i)=Vec3b(0,0,255);
			}
		}
	}

	imshow("I1",I1copie);
	imshow("I2",I2copie);
	waitKey();

	int DIFF=DIFF_ROWS;

	int taille=hauteur;
	
	//constante représentant l'infini
	const int MAX_INT=10000;

	//creation du graphe de flot
	Graph<double,double,double> g(LARG*HAUT,4*LARG*HAUT);
	g.add_node(LARG*HAUT);
	int ii;
	for (int i=0; i<HAUT; i++)
	{
		ii=i+abs(DIFF_ROWS);
		for (int j=0; j<LARG; j++)
		{
			int pix = i*LARG+j;
			double c;
			if (preflot.at<uchar>(ii,j)==2)
			{
				g.add_tweights(pix,MAX_INT,0);
			}
			if (preflot.at<uchar>(ii,j)==1)
			{
				g.add_tweights(pix,0,MAX_INT);
			}
			if (i!=HAUT-1)
			{	
				c=cost2(I1,I2,ii,j,ii+1,j,Gx1,Gy1,Gx2,Gy2);
				g.add_edge(pix, (i+1)*LARG+j,c,c);
			}
			if (j!=LARG-1)
			{
				c=cost2(I1,I2,ii,j,ii,j+1,Gx1,Gy1,Gx2,Gy2);
				g.add_edge(pix,i*LARG+j+1,c,c);
			}
		}
	}
	
	double flow=g.maxflow();
	//cout<<flow<<endl;

	//image resultat
	Mat res(hauteur,largeur, CV_8UC3);
	//Gestion des pixels au bord des images:
	//comme les images d'origine n'ont pas forcément la même taille on doit faire attention
	//aux pixels des bouts des deux images qui ne se recouvrent pas
	for (int i=0; i<abs(DIFF_ROWS); i++)
	{
		for (int j=0; j<largeur; j++)
		{
			if (DIFF_ROWS<0)
			{
				res.at<Vec3b>(i,j)=I2.at<Vec3b>(i,j);
			}
			if (DIFF_ROWS>0)
			{
				res.at<Vec3b>(i,j)=I1.at<Vec3b>(i,j);
			}

		}
	}
	for (int j=0; j<abs(DIFF_COLS); j++)
	{
		for (int i=0; i<hauteur; i++)
		{
			if (DIFF_COLS<0)
			{
				res.at<Vec3b>(i,j+LARG)=I2.at<Vec3b>(i,j+LARG);
			}
			if (DIFF_COLS>0)
			{
				res.at<Vec3b>(i,j+LARG)=I1.at<Vec3b>(i,j+LARG);
			}

		}
	}

	//Remplissage de l'image résultat
	for (int i=0; i<HAUT; i++)
	{
		for (int j=0; j<LARG; j++)
		{
			int pix=i*LARG+j;
			if (g.what_segment(pix)==Graph<double,double,double>::SINK)
			{
				res.at<Vec3b>(i+abs(DIFF_ROWS),j)=I1.at<Vec3b>(i+abs(DIFF_ROWS),j);
			}
			else
			{
				res.at<Vec3b>(i+abs(DIFF_ROWS),j)=I2.at<Vec3b>(i+abs(DIFF_ROWS),j);
			}
		}
	}

	
	imshow("Résultat",res);
	waitKey();
	return 0;

}
