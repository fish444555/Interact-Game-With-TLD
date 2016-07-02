#include <opencv2/opencv.hpp>
#include <tld_utils.h>
#include <iostream>
#include <sstream>
#include <TLD.h>
#include <stdio.h>

#include <linux/input.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

using namespace cv;
using namespace std;
//Global variables
Rect box;
bool drawing_box = false;
bool gotBB = false;
bool tl = false;
bool rep = false;
bool fromfile=false;
string video;


//xiaoyi added here  
// String fist_cascade = "c1_lbp_900_20.xml";  
// String palm_cascade = "palmcascade2.xml"; 

void readBB(char* file){
  ifstream bb_file (file);
  string line;
  getline(bb_file,line);
  istringstream linestream(line);
  string x1,y1,x2,y2;
  getline (linestream,x1, ',');
  getline (linestream,y1, ',');
  getline (linestream,x2, ',');
  getline (linestream,y2, ',');
  int x = atoi(x1.c_str());// = (int)file["bb_x"];
  int y = atoi(y1.c_str());// = (int)file["bb_y"];
  int w = atoi(x2.c_str())-x;// = (int)file["bb_w"];
  int h = atoi(y2.c_str())-y;// = (int)file["bb_h"];
  box = Rect(x,y,w,h);
}
//bounding box mouse callback
void mouseHandler(int event, int x, int y, int flags, void *param){
  switch( event ){
  case CV_EVENT_MOUSEMOVE:
    if (drawing_box){
        box.width = x-box.x;
        box.height = y-box.y;
    }
    break;
  case CV_EVENT_LBUTTONDOWN:
    drawing_box = true;
    box = Rect( x, y, 0, 0 );
    break;
  case CV_EVENT_LBUTTONUP:
    drawing_box = false;
    if( box.width < 0 ){
        box.x += box.width;
        box.width *= -1;
    }
    if( box.height < 0 ){
        box.y += box.height;
        box.height *= -1;
    }
    gotBB = true;
    break;
  }
}

void print_help(char** argv){
  printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
  printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n-r     repeat\n");
}

void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
  for (int i=0;i<argc;i++){
      if (strcmp(argv[i],"-b")==0){
          if (argc>i){
              readBB(argv[i+1]);
              gotBB = true;
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-s")==0){
          if (argc>i){
              video = string(argv[i+1]);
              capture.open(video);
              fromfile = true;
          }
          else
            print_help(argv);

      }
      if (strcmp(argv[i],"-p")==0){
          if (argc>i){
              fs.open(argv[i+1], FileStorage::READ);
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-tl")==0){
          tl = true;
      }
      if (strcmp(argv[i],"-r")==0){
          rep = true;
      }
  }
}


void simulate_key(int fd, int kval)//keyboard
{
    struct input_event event;
    gettimeofday(&event.time, 0);
    //按下kval键
    event.type = EV_KEY;
    event.value = 1;
    event.code = kval;
    write(fd, &event, sizeof(event));
    //同步，也就是把它报告给系统
    event.type = EV_SYN;
    event.value = 0;
    event.code = SYN_REPORT;
    write(fd, &event, sizeof(event));

    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, 0);
    //松开kval键
    event.type = EV_KEY;
    event.value = 0;
    event.code = kval;
    write(fd, &event, sizeof(event));
    //同步，也就是把它报告给系统
    event.type = EV_SYN;
    event.value = 0;
    event.code = SYN_REPORT;
    write(fd, &event, sizeof(event));
}
//鼠标移动模拟
void simulate_mouse(int fd, int rel_x, int rel_y)
{
    struct input_event event;
    gettimeofday(&event.time, 0);
    //x轴坐标的相对位移
    event.type = EV_REL;
    event.value = rel_x;
    event.code = REL_X;
    write(fd, &event, sizeof(event));
    //y轴坐标的相对位移
    event.type = EV_REL;
    event.value = rel_y;
    event.code = REL_Y;
    write(fd, &event, sizeof(event));
    //同步
    event.type = EV_SYN;
    event.value = 0;
    event.code = SYN_REPORT;
    write(fd, &event, sizeof(event));
}

void simulate_mouse2(int fd)
{
    struct input_event event;
    gettimeofday(&event.time, 0);
    //x轴坐标的相对位移
    event.type = EV_KEY;
    event.value = 1;
    event.code = BTN_LEFT;
    write(fd, &event, sizeof(event));
    //y轴坐标的相对位移
    event.type = EV_KEY;
    event.value = 0;
    event.code = BTN_LEFT;
    write(fd, &event, sizeof(event));
    //同步
    event.type = EV_SYN;
    event.value = 0;
    event.code = SYN_REPORT;
    write(fd, &event, sizeof(event));
}


//added here  
int detect_object( Mat& img, CascadeClassifier& cascade, vector<Rect> &objects, float scale)  
{      
    // Scale down the image, speed up tracking process
    Mat smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1 );   
    //size Scale down to 1/scale
    resize( img, smallImg, smallImg.size(), 0, 0, INTER_LINEAR );   
    equalizeHist( smallImg, smallImg );
    cascade.detectMultiScale( smallImg, objects, 1.1, 2, CV_HAAR_SCALE_IMAGE, Size(30, 30) );  
     
    return objects.size();  
}  
  
//added here  
int detect_fist( Mat& img)  
{  
    vector<Rect> fists;  
    double scale = 1.3;  
    Mat gray;  
    CascadeClassifier fist_cascade;//create CascadeClassifier object
    if( !fist_cascade.load( "/media/User_Data/fish/xp_vb/share/Final/dataset/QQ/fist.dat" ) ) //从指定的文件目录中加载级联分类器  
    {  
        cerr << "ERROR: Could not load classifier cascade" << endl;  
        return 0;  
    }  
    cvtColor( img, gray, CV_BGR2GRAY );  
    detect_object( gray, fist_cascade, fists, scale);   
    if (fists.size())  {
      printf("the fist.size is---------------------------------%d \n", fists.size());       
    }
  
    return fists.size()>0;  
}  
  
//added here  
int detect_palm_first(Mat &img)  
{  
    CascadeClassifier cascade;
    double scale = 1.3;  
    vector<Rect> palms;  
    Mat gray;  
      
    // if( !cascade.load( "/media/User_Data/fish/tools/opencv_2.4.9/opencv-2.4.9/data/lbpcascades/lbpcascade_frontalface.xml" ) ) //从指定的文件目录中加载级联分类器  
    // if( !cascade.load( "/media/User_Data/fish/xp_vb/share/Final/dataset/QQ/digits_svm.dat" ) ) //从指定的文件目录中加载级联分类器  
    if( !cascade.load( "./palm.dat" ) ) 
    {  
        cerr << "ERROR: Could not load classifier cascade" << endl;  
        return 0;  
    }  
    //haar feature base on gray image, so tranform to gray image    
    cvtColor( img, gray, CV_BGR2GRAY );  
    detect_object( gray, cascade, palms, scale);      
     
    if (!palms.size())  
        return 0;  
  
    box.width = cvRound( palms[0].width * scale * 0.7 );  
    box.height = cvRound( palms[0].height * scale * 0.8 );  
    box.x = cvRound(palms[0].x * scale + palms[0].width * scale * 0.15 );  
    box.y = cvRound(palms[0].y * scale +  palms[0].height * scale * 0.1 );  
  
    gotBB = true;  
    printf("detect objects-------------------------");
    return 1;  
}  

int main(int argc, char * argv[]){
  VideoCapture capture;
  capture.open(0);
  FileStorage fs;
  //Read options
  read_options(argc,argv,capture,fs);
  //Init camera
  if (!capture.isOpened())
  {
	cout << "capture device failed to open!" << endl;
    return 1;
  }
  //Register mouse callback to draw the bounding box
  cvNamedWindow("TLD",CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback( "TLD", mouseHandler, NULL );
  //TLD framework
  TLD tld;
  //Read parameters file
  tld.read(fs.getFirstTopLevelNode());
  Mat frame;
  Mat last_gray;
  Mat first;
  if (fromfile){
      capture >> frame;
      cvtColor(frame, last_gray, CV_RGB2GRAY);
      frame.copyTo(first);
  }else{
      capture.set(CV_CAP_PROP_FRAME_WIDTH,340);
      capture.set(CV_CAP_PROP_FRAME_HEIGHT,240);
  }

  ///Initialization
GETBOUNDINGBOX:
  while(!gotBB)
  {
    if (!fromfile){
      capture >> frame;
    }
    else
      first.copyTo(frame);
    cvtColor(frame, last_gray, CV_RGB2GRAY);
    // detect_palm_first(frame); //added here
    drawBox(frame,box);
    imshow("TLD", frame);
    if (cvWaitKey(33) == 'q')
	    return 0;
  }
  if (min(box.width,box.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
      cout << "Bounding box too small, try again." << endl;
      gotBB = false;
      goto GETBOUNDINGBOX;
  }
  //Remove callback
  cvSetMouseCallback( "TLD", NULL, NULL );
  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height);
  //Output file
  FILE  *bb_file = fopen("bounding_boxes.txt","w");
  //TLD initialization
  tld.init(last_gray,box,bb_file);

  //added here  mouse & keyboard control
  int fd_kbd = -1;
  int fd_mouse = -1;
  int open_success = 1;
  fd_kbd = open("/dev/input/event3", O_RDWR);
  if(fd_kbd <= 0){
      printf("Can not open keyboard input file\n");
      open_success = 0;
  }
  fd_mouse = open("/dev/input/event4", O_RDWR);
  if(fd_mouse <= 0){
      printf("Can not open mouse input file\n");
      open_success = 0;
  }

  ///Run-time
  Mat current_gray;
  BoundingBox pbox;

  BoundingBox tbox;
  tbox.x = -1; tbox.y = -1;
  int x_pixel_move = 0;
  int y_pixel_move = 0;

  vector<Point2f> pts1;
  vector<Point2f> pts2;
  bool status=true;
  int frames = 1;
  int detections = 1;
REPEAT:
  while(capture.read(frame)){
    //get frame    
    cvtColor(frame, current_gray, CV_RGB2GRAY);
    //Process Frame
    tld.processFrame(last_gray,current_gray,pts1,pts2,pbox,status,tl,bb_file);
    //Draw Points
    // detect_fist(frame);
    
    if (detect_fist(frame)){ //added here
          simulate_mouse2(fd_mouse);
    }
    if (detect_palm_first(frame)){ //added here
      ;
    }
    if (status){
      drawPoints(frame,pts1);
      drawPoints(frame,pts2,Scalar(0,255,0));
      drawBox(frame,pbox);
      detections++;
      // printf("capture\n");

      //added here
      if (tbox.x == -1){
        tbox = pbox;
      }
      else{
        x_pixel_move =(int)( (tbox.x + tbox.width)/2 - (pbox.x + pbox.width)/2);
        y_pixel_move =(int)( (pbox.y + pbox.height)/2 - (tbox.y + tbox.height)/2);
        if (norm(x_pixel_move) > 2 || norm(y_pixel_move) > 2 ){//avoid fluctuation
            simulate_mouse(fd_mouse, 10 * x_pixel_move, 10 * y_pixel_move);
          // if (x_pixel_move>0){
          //   simulate_key(fd_kbd, KEY_RIGHT);
          // }
          // else{
          //   simulate_key(fd_kbd, KEY_LEFT);
          // }
          // if (y_pixel_move>0){
          //   simulate_key(fd_kbd, KEY_DOWN);
          // }
          // else{
          //   simulate_key(fd_kbd, KEY_UP); 
          // }        

        }
        
        tbox = pbox;

        printf("mouse\n");
      }
      /*
      if (open_success && (x_pixel_move > 8 || x_pixel_move < -8 || y_pixel_move > 8 || y_pixel_move < -8)){
         if (x_pixel_move < -8)
           simulate_key(fd_kbd, KEY_RIGHT);
         else if (x_pixel_move > 8)
           simulate_key(fd_kbd, KEY_LEFT);
         else if (y_pixel_move < -8)
           simulate_key(fd_kbd, KEY_UP);
         else if (y_pixel_move > 8)
           simulate_key(fd_kbd, KEY_DOWN);

         tbox = pbox;
         x_pixel_move = 0;
         y_pixel_move = 0;
      }
      */
    }
    // else{// fish add
    //   gotBB = false;
    //   fclose(bb_file);
    //   goto GETBOUNDINGBOX;
    // }
    //Display
    imshow("TLD", frame);
    //swap points and images
    swap(last_gray,current_gray);
    pts1.clear();
    pts2.clear();
    frames++;
    // printf("Detection rate: %d/%d\n",detections,frames); fish del
    if (cvWaitKey(33) == 'q')
      break;
  }
  if (rep){
    rep = false;
    tl = false;
    fclose(bb_file);
    bb_file = fopen("final_detector.txt","w");
    //capture.set(CV_CAP_PROP_POS_AVI_RATIO,0);
    capture.release();
    capture.open(video);
    goto REPEAT;
  }
  fclose(bb_file);
  return 0;
}
