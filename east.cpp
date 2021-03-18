#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
using namespace cv;
using namespace cv::dnn;
using namespace std;
void decode(const Mat& scores, const Mat& geometry, float scoreThresh,
    std::vector<RotatedRect>& detections, std::vector<float>& confidences);
//String model = "C:/Users/XHG/Desktop/xiangmu/frozen_east_text_detection.pb";
String model = "frozen_east_text_detection.pb";
Net net;// = readNet(model);
void eastInit()
{
    net = readNet(model);
}
bool east(Mat &frame, vector<RotatedRect> &outboxes)
{
   // imwrite("data.bmp",frame);
    //frame = frame(Rect(0, 0, frame.cols, frame.rows / 4));
    outboxes.clear();
    float confThreshold = 0.99;
    float nmsThreshold = 0.4;
    int inpWidth = 256;//320
    int inpHeight = 64;//320
    //    cvtColor(frame,frame,CV_RGBA2RGB);
    auto names = net.getLayerNames();

    /*net.setPreferableBackend(DNN_BACKEND_OPENCV);
    net.setPreferableTarget(DNN_TARGET_OPENCL);*/
    //static const std::string kWinName = "EAST: An Efficient and Accurate Scene Text Detector";
    //namedWindow(kWinName, WINDOW_NORMAL);

    // 设定网络提取层的数据
    vector<RotatedRect> boxes;
    std::vector<Mat> outs;
    std::vector<String> outNames(2);
    outNames[0] = "feature_fusion/Conv_7/Sigmoid";
    outNames[1] = "feature_fusion/concat_3";

    Mat  blob;
    // 输入图片、网络前向计算
    double fre = getTickFrequency();
    double t1 = getTickCount() / fre;
    blobFromImage(frame, blob, 1.0, Size(inpWidth, inpHeight), Scalar(123.68, 116.78, 103.94), true, false);
    net.setInput(blob);
    net.forward(outs, outNames);

    Mat scores = outs[0];    // 1x1x80x80
    Mat geometry = outs[1];  // 1x5x80x80

    // 输出数据Blob转换为可操作的数据对象

    std::vector<float> confidences;
    decode(scores, geometry, confThreshold, boxes, confidences);

    // NMS处理检测结果
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

    // 绘制检测结果
    Point2f ratio((float)frame.cols / inpWidth, (float)frame.rows / inpHeight);
  //  Mat sframe = frame.clone();
    for (int indice : indices) {
        RotatedRect box = boxes[indice];

        Point2f vertices[4];
        //box.points(vertices);
        //// 映射(inpwidth,inpheight)到输入图像实际大小比例中
        //for (auto & vertice : vertices) {
        //	vertice.x *= ratio.x;
        //	vertice.y *= ratio.y;
        //}
        //for (int j = 0; j < 4; ++j)
        //	line(frame, vertices[j], vertices[(j + 1) % 4], scalar(0, 255, 0), 1);
        box.center.x *= ratio.x;
        box.center.y *= ratio.y;
        box.size.width *= ratio.x;
        box.size.height *= ratio.y;
        box.points(vertices);

   //     for (int j = 0; j < 4; ++j)
       //     line(sframe, vertices[j], vertices[(j + 1) % 4], Scalar(0, 255, 0), 1);
        outboxes.push_back(box);
    }

    // 相关检测效率信息
            std::vector<double> layersTimes;
            double freq = getTickFrequency() / 1000;
            double t = net.getPerfProfile(layersTimes) / freq;
    // std::string label = format("Inference time: %.2f ms", t);
    //putText(frame, label, Point(0, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0));
    // static int n=1;
    //   imwrite(to_string(n++)+".png",sframe);
  //    imshow("sframe", sframe);
    cout <<endl<< "stime:" << (getTickCount() / fre - t1) / 1 << "  box num:" << outboxes.size() << "  infTime:"<<t;
    double totol = 0;
    for (auto kk : confidences)
    {
        totol += kk;
    }
    if (confidences.size()>0)
        cout <<"  prob:"<< totol / confidences.size();
    if (outboxes.size() > 0)
    {
        //    cout << "boxsize:" << outboxes.size() << endl;
        return 1;
    }
    return 0;
}

void decode(const Mat& scores, const Mat& geometry, float scoreThresh,
    std::vector<RotatedRect>& detections, std::vector<float>& confidences)
{
    detections.clear();
    CV_Assert(scores.dims == 4);	CV_Assert(geometry.dims == 4);
    CV_Assert(scores.size[0] == 1);	CV_Assert(geometry.size[0] == 1);
    CV_Assert(scores.size[1] == 1); CV_Assert(geometry.size[1] == 5);
    CV_Assert(scores.size[2] == geometry.size[2]);
    CV_Assert(scores.size[3] == geometry.size[3]);

    const int height = scores.size[2];
    const int width = scores.size[3];
    for (int y = 0; y < height; ++y) {
        // 各行像素点对应的 score、4个距离、角度的 数据指针
        const auto* scoresData = scores.ptr<float>(0, 0, y);
        const auto* x0_data = geometry.ptr<float>(0, 0, y);
        const auto* x1_data = geometry.ptr<float>(0, 1, y);
        const auto* x2_data = geometry.ptr<float>(0, 2, y);
        const auto* x3_data = geometry.ptr<float>(0, 3, y);
        const auto* anglesData = geometry.ptr<float>(0, 4, y);
        for (int x = 0; x < width; ++x) {
            float score = scoresData[x];       // score
            if (score < scoreThresh)
                continue;

            // 输入图像经过网络有4次缩小
            float offsetX = x * 4.0f, offsetY = y * 4.0f;
            float angle = anglesData[x];       // 外接矩形框旋转角度
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            float h = x0_data[x] + x2_data[x]; // 外接矩形框高度
            float w = x1_data[x] + x3_data[x]; // 外接矩形框宽度

            // 通过外接矩形框，旋转角度，建立旋转矩形
            Point2f offset(offsetX + cosA * x1_data[x] + sinA * x2_data[x],
                offsetY - sinA * x1_data[x] + cosA * x2_data[x]);
            Point2f p1 = Point2f(-sinA * h, -cosA * h) + offset;
            Point2f p3 = Point2f(-cosA * w, sinA * w) + offset;
            RotatedRect r(0.5f * (p1 + p3), Size2f(w, h), -angle * 180.0f / (float)CV_PI);
            detections.push_back(r);
            confidences.push_back(score);
        }
    }
}
