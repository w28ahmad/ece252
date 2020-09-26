echo "test 1"
./catpng.out
echo "test 2"
sleep 1
./catpng.out ./pics/pic_1.png ./pics/pic_2.png ./pics/pic_3.png
xdg-open all.png
echo "test 3"
sleep 1
./catpng.out ./pics/pic2_1.png ./pics/pic2_2.png ./pics/pic2_3.png ./pics/pic2_4.png
echo "test 4"
sleep 1
./catpng.out ./pics/pic_1.png
echo "test 5"
sleep 1
./catpng.out ./pics/pic2_1.png
echo "test 6"
sleep 1
./catpng.out ./pics/pic3_1.png
echo "test 7"
sleep 1
./catpng.out ./pics/pic3_1.png ./pics/pic3_2.png
echo "test 8"
sleep 1
./catpng.out ./pics/pic3_1.png ./pics/pic3_2.png ./pics/pic3_3.png ./pics/pic3_4.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png ./pics/pic3_5.png

echo "crop test 1"
sleep 1
./catpng.out ./pics/cropped/pic_cropped/pic_cropped*
echo "crop test 2"
sleep 1
./catpng.out ./pics/uweng_cropped/uweng_cropped*

