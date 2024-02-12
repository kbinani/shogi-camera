#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgcodecs/ios.h>
#include <opencv2/core/types_c.h>

#include <vector>

namespace com::github::kbinani::sci {
    
    namespace {
    int const N = 5;
    int const thresh = 50;
    
    double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0) {
        double dx1 = pt1.x - pt0.x;
        double dy1 = pt1.y - pt0.y;
        double dx2 = pt2.x - pt0.x;
        double dy2 = pt2.y - pt0.y;
        return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
    }
    }
    
    void findSquares(cv::Mat const& image, std::vector<std::vector<cv::Point2i>> &squares) {
        cv::Mat timg(image);
        cv::cvtColor(image, timg, CV_RGB2GRAY);
        
        cv::Mat gray0(image.size(), CV_8U), gray;
        
        std::vector<std::vector<cv::Point> > contours;
        cv::Size size = image.size();
        double area = size.width * size.height;
        
        int ch[] = {0, 0};
        mixChannels(&timg, 1, &gray0, 1, ch, 1);
        
        for (int l = 0; l < N; l++) {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading
            if (l == 0) {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging)
                Canny(gray0, gray, 5, thresh, 5);
                // dilate canny output to remove potential
                // holes between edge segments
                dilate(gray, gray, cv::Mat(), cv::Point(-1,-1));
            } else {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                gray = gray0 >= (l+1)*255/N;
            }
            
            // find contours and store them all as a list
            findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
            
            std::vector<cv::Point> approx;
            
            // test each contour
            for (size_t i = 0; i < contours.size(); i++) {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                approxPolyDP(cv::Mat(contours[i]), approx, arcLength(cv::Mat(contours[i]), true)*0.02, true);
                
                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                double cArea = fabs(contourArea(cv::Mat(approx)));
                if (area * 0.01 < cArea && cArea < area * 0.9 && isContourConvex(cv::Mat(approx))) {
                    double maxCosine = 0;
                    
                    for (int j = 2; j < 5; j++) {
                        // find the maximum cosine of the angle between joint edges
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }
                    
                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if (maxCosine < 0.3) {
                        squares.push_back(approx);
                    }
                }
            }
        }
        
        cv::Point2f center = cv::Point2f(size.width / 2.0f, size.height / 2.0f);
        std::sort(squares.begin(), squares.end(), [center](std::vector<cv::Point> const& a, std::vector<cv::Point> const& b) {
            cv::Moments mA = cv::moments(a);
            cv::Moments mB = cv::moments(b);
            cv::Point2f m2A = cv::Point2f(mA.m10 / mA.m00, mA.m01 / mA.m00);
            cv::Point2f m2B = cv::Point2f(mB.m10 / mB.m00, mB.m01 / mB.m00);
            double distanceA = hypot(m2A.x - center.x, m2A.y - center.y);
            double distanceB = hypot(m2B.x - center.x, m2B.y - center.y);
            return distanceA < distanceB;
        });
        
        bool changed = true;
        while (changed && squares.size() > 1) {
            changed = false;
            for (size_t i = 0; i < squares.size() - 1; i++) {
                auto const& a = squares[i];
                auto const& b = squares[i + 1];
                cv::Moments mA = cv::moments(a);
                cv::Moments mB = cv::moments(b);
                cv::Point2f m2A = cv::Point2f(mA.m10 / mA.m00, mA.m01 / mA.m00);
                cv::Point2f m2B = cv::Point2f(mB.m10 / mB.m00, mB.m01 / mB.m00);
                double areaA = fabs(contourArea(a));
                double areaB = fabs(contourArea(b));
                double distance = hypot(m2A.x - m2B.x, m2A.y - m2B.y);
                if (fabs(areaA - areaB) / ((areaA + areaB) / 2) < 0.1 && distance < MIN(size.width, size.height) * 0.1) {
                    // remove B
                    changed = true;
                    squares.erase(squares.begin() + i + 1);
                    break;
                }
            }
        }
        
        double distanceThreshold = MIN(size.width, size.height) * 0.1;
        
        changed = true;
        while (changed && squares.size() > 1) {
            changed = false;
            for (size_t i = 0; i < squares.size() - 1; i++) {
                auto const& points = squares[i];
                int num = 0; // 画像の四辺の上に重なっている辺の個数.
                for (size_t j = 0; j < 3; j++) {
                    auto const& a = points[j];
                    auto const& b = points[j + 1];
                    
                    // 上辺
                    if (fabs(a.y) < distanceThreshold && fabs(b.y) < distanceThreshold) {
                        num++;
                        continue;
                    }
                    
                    // 下辺
                    if (fabs(a.y - size.height) < distanceThreshold && fabs(b.y - size.height) < distanceThreshold) {
                        num++;
                        continue;
                    }
                    
                    // 左辺
                    if (fabs(a.x) < distanceThreshold && fabs(b.x) < distanceThreshold) {
                        num++;
                        continue;
                    }
                    
                    // 右辺
                    if (fabs(a.x - size.width) < distanceThreshold && fabs(b.x - size.width) < distanceThreshold) {
                        num++;
                        continue;
                    }
                }
                if (num >= 2) {
                    changed = true;
                    squares.erase(squares.begin() + i);
                    break;
                }
            }
        }
    }
    
}
