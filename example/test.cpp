#include "../include/inout.hpp"

#include <cmath>
#include <iostream>
#include <exception>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/format.hpp>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>
#include <limits>
#include <type_traits>

using namespace inout;


class ImageException
    : public boost::exception,
      public std::exception{};

typedef boost::error_info<struct err_info, std::string> ImageAddInfo;


enum class Direction{
    right, up, left, down
};


template <typename T>
auto vec_total(T const & vec, std::size_t size)
    -> typename std::remove_const<typename std::remove_reference<decltype(vec[size])>::type>::type
{
    typename std::remove_const<typename std::remove_reference<decltype(vec[0])>::type>::type sum = 0;

    for(std::size_t i = 1; i < size; ++i)
        sum += std::abs(vec[i]);
    return sum;
}


/**
ある画像img1に対して、方角directionに画像img2がどの程度相関があるかを返しますｔ
*/
template <typename T, typename U, typename std::enable_if<is_image((T*)nullptr) && is_image((U*)nullptr)>::type *& = enabler_ptr>
double diff_connection(T const & img1, U const & img2, Direction direction)

{
    double sum = 0;

    if(img1.height() != img2.height() || img1.width() != img2.width())
        BOOST_THROW_EXCEPTION(ImageException()
            << ImageAddInfo("2つの引数の画像は、それぞれ異なる大きさを持っています")
            << ImageAddInfo((boost::format("img1.size <- (%1%, %2%)") % img1.width() % img1.height()).str())
            << ImageAddInfo((boost::format("img2.size <- (%1%, %2%)") % img2.width() % img2.height()).str()));


    std::size_t r1 = 0, c1 = 0, r2 = 0, c2 = 0;
    switch(direction)
    {
      case Direction::right:
        c1 = img1.width() - 1;
        c2 = 0;
        break;

      case Direction::up:
        r1 = 0;
        r2 = img2.height() - 1;
        break;

      case Direction::left:
        c1 = 0;
        c2 =  img2.width() - 1;
        break;

      case Direction::down:
        r1 = img1.height() - 1;
        r2 = 0;
        break;
    }

    switch(direction)
    {
      case Direction::right:
      case Direction::left:
        for(std::size_t r = 0; r < img1.height(); ++r)
            sum += vec_total(img1.get_pixel(r, c1).vec() - img2.get_pixel(r, c2).vec(), 3);
        break;

      case Direction::up:
      case Direction::down:
        for(std::size_t c = 0; c < img1.width(); ++c)
            sum += vec_total(img1.get_pixel(r1, c).vec() - img2.get_pixel(r2, c).vec(), 3);
        break;
    }

    return sum;
}



int main()
{
    auto p_opt = Problem::get_from_test_server(1);

    if(p_opt){
        const Problem& p = *p_opt;

        std::cout << "縦分割数 : " << p.div_x() << std::endl;
        std::cout << "横分割数 : " << p.div_y() << std::endl;

        std::cout << "交換コスト : " << p.change_cost() << std::endl;
        std::cout << "選択コスト : " << p.select_cost() << std::endl;

        std::cout << "最大選択可能回数 : " << p.max_select_times() << std::endl;


        // 一番スミにある(0, 0)要素の4方向にどの画像がくっつくかを調べる
        for(int i = 0; i < 4; ++i){
            double m = std::numeric_limits<double>::infinity();
            std::size_t rm, cm;

            for(std::size_t r = 0; r < p.div_y(); ++r)
                for(std::size_t c = 0; c < p.div_x(); ++c){
                    if(r == 2 && c == 1)
                        continue;

                    double v = diff_connection(p.get_element(2, 1), p.get_element(r, c), static_cast<Direction>(i));
                    m = std::min(m, v);

                    if(m == v){
                        rm = r;
                        cm = c;
                    }
                }

            std::cout << boost::format("部分画像(%1%, %2%)の") % 2 % 1;
            switch(i){
                case 0: std::cout << "右"; break;
                case 1: std::cout << "上"; break;
                case 2: std::cout << "左"; break;
                case 3: std::cout << "下"; break;
            }

            std::cout << boost::format("には、部分画像(%1%, %2%)がベストマッチです。") % rm % cm << std::endl;
        }

        auto& src_img = p.cvMat();
        
        cv::namedWindow("image1", cv::WINDOW_AUTOSIZE|cv::WINDOW_FREERATIO);
        
        // ウィンドウ名でウィンドウを指定して，そこに画像を描画
        cv::imshow("image1", src_img);
       
        // デフォルトのプロパティで表示
        // cv::imshow("image2", src_img);
        if(!send_result_to_test_server(1, "k3kaimu", "k3foobar", "2\r\n11\r\n21\r\nURDDLLURRDLLUURDDLUUD\r\n11\r\n40\r\nURDLURLDLURDRDLURUDLURDLLRDLUURRDLLURRDL\r\n"))
            std::cout << "死にました" << std::endl;

        // キー入力を（無限に）待つ
        cv::waitKey(0);
    }else
        std::cout << "死" << std::endl;
}
