#include "../include/inout.hpp"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


using namespace inout;

int main()
{
    auto p_opt = Problem::get_from_test_server(1);

    if(p_opt){
        const Problem& p = *p_opt;

        std::cout << p.div_x() << std::endl;
        std::cout << p.div_y() << std::endl;

        std::cout << p.change_cost() << std::endl;
        std::cout << p.select_cost() << std::endl;

        std::cout << p.max_select_times() << std::endl;

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
