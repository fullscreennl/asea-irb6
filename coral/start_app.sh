while true
do
python3 serial_face_detector.py --model ./all_models/mobilenet_ssd_v2_face_quant_postprocess_edgetpu.tflite
sleep 1
done
