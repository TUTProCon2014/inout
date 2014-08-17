#include "../include/inout.hpp"

#include <cmath>
#include <iostream>
#include <exception>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <limits>
#include <type_traits>
#include <boost/format.hpp>
#include "../../utils/include/constants.hpp"
#include "../../utils/include/template.hpp"
#include "../../utils/include/types.hpp"

using namespace procon::utils;
using namespace procon::inout;


template <typename T>
auto vec_total(T const & vec, std::size_t size)
    -> typename std::remove_const<typename std::remove_reference<decltype(vec[size])>::type>::type
{
    decltype(vec_total(vec, size)) sum = 0;

    for(std::size_t i = 1; i < size; ++i)
        sum += std::abs(vec[i]);

    return sum;
}


/**
ある画像img1に対して、方角directionに画像img2がどの程度相関があるかを返します
*/
#ifdef NOT_SUPPORT_CONSTEXPR
template <typename T, typename U>
#else
template <typename T, typename U,
    PROCON_TEMPLATE_CONSTRAINTS(is_image<T>() && is_image<U>())>   // T, Uともに画像であるという制約
#endif
double diff_connection(T const & img1, U const & img2, Direction direction)
{
    double sum = 0;

    if(img1.height() != img2.height() || img1.width() != img2.width())
        return std::numeric_limits<double>::infinity();


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


#ifndef NOT_SUPPORT_CONSTEXPR
/// ditto
template <typename T, typename U,
    PROCON_TEMPLATE_CONSTRAINTS(!is_image<T>() || !is_image<U>())>
double diff_connection(T const & img1, U const & img2, Direction direction)
{
    static_assert(is_image<T>(), "1st argument is not a image type value");
    static_assert(is_image<U>(), "2nd argument is not a image type value");

    return 0;
}
#endif


int main()
{
    auto p_opt = get_problem_from_test_server(1);

#ifndef NOT_SUPPORT_CONSTEXPR
    static_assert(is_image<Problem>(), "NG: Problem");
    static_assert(is_image<Problem&>(), "NG: Problem&");
    static_assert(is_image<Problem const &>(), "NG: Problem const &");
    static_assert(!is_image<Problem*>(), "NG: Problem* is not a member of image type");
    static_assert(is_image<ElementImage>(), "NG: ElementImage");
    static_assert(is_image<ElementImage&>(), "NG: ElementImage&");
    static_assert(is_image<ElementImage const &>(), "NG: ElementImage const &");
    static_assert(!is_image<ElementImage*>(), "NG: ElementImage* is not a member of image type");
    static_assert(!is_image<int>(), "NG: int is not a member of image type");
    static_assert(!is_image<double>(), "NG:  double is not a member of image type");
    static_assert(!is_image<std::string>(), "NG:  double is not a member of image type");
    static_assert(!is_image<std::vector<int>>(), "NG:  double is not a member of image type");

    static_assert(!is_image<decltype(p_opt)>(), "NG: decltype(p_opt) is not a member of image type");
    static_assert(is_image<decltype(*p_opt)>(), "NG: decltype(*p_opt)");
#endif

    if(p_opt){
        const Problem& p = *p_opt;

        std::cout << "div_x : " << p.div_x() << std::endl;
        std::cout << "div_y : " << p.div_y() << std::endl;

        std::cout << "change_cost : " << p.change_cost() << std::endl;
        std::cout << "select_cost : " << p.select_cost() << std::endl;

        std::cout << "max_select_times : " << p.max_select_times() << std::endl;


        // 一番スミにある(0, 0)要素の4方向にどの画像がくっつくかを調べる
        for(int i = 0; i < 4; ++i){
            double m = std::numeric_limits<double>::infinity();
            std::size_t rm, cm;

            for(std::size_t r = 0; r < p.div_y(); ++r)
                for(std::size_t c = 0; c < p.div_x(); ++c){
                    if(r == 2 && c == 1)
                        continue;

                    double v = diff_connection(p.get_element(2, 1), p.get_element(r, c), static_cast<Direction>(i));
                    // auto vv = diff_connection(1, 1, static_cast<Direction>(i));  // NG
                    m = std::min(m, v);

                    if(m == v){
                        rm = r;
                        cm = c;
                    }
                }

            std::cout << boost::format("(%1%, %2%) ") % 2 % 1;
            switch(i){
                case 0:
                    std::cout << "R" ;
                    break;
                case 1:
                    std::cout << "U" ;
                    break;
                case 2:
                    std::cout << "L" ;
                    break;
                case 3:
                    std::cout << "D" ;
                    break;
            }

            std::cout << boost::format("-> (%1%, %2%)") % rm % cm << std::endl;
        }

        auto& src_img = p.cvMat();
        
        cv::namedWindow("image1", cv::WINDOW_AUTOSIZE);
        
        // ウィンドウ名でウィンドウを指定して，そこに画像を描画
        cv::imshow("image1", src_img);
       
        // デフォルトのプロパティで表示
        // cv::imshow("image2", src_img);
        if(!send_result_to_test_server(1, "k3kaimu", "k3foobar", "2\r\n11\r\n21\r\nURDDLLURRDLLUURDDLUUD\r\n11\r\n40\r\nURDLURLDLURDRDLURUDLURDLLRDLUURRDLLURRDL\r\n"))
            std::cout << "fueeeee" << std::endl;

        // キー入力を（無限に）待つ
        cv::waitKey(0);
    }else
        std::cout << "fueee" << std::endl;
}
