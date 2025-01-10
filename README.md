# License Plate and Province Detection using EasyOCR and Yolov8

This project uses **EasyOCR and Yolov8** to detect and extract license plates and provinces from images of vehicle license plates in Thailand. It utilizes OpenCV for image preprocessing and regex for filtering valid license plates and province names.

## Features
- Extracts text from images using **EasyOCR**.
- Detects Thai **license plates** in the format `กท-152` or `ดล-5831`.
- Identifies **provinces** in Thailand from a predefined list.
- Provides clean, filtered results by removing unwanted characters and non-relevant data.

## Technologies
- **EasyOCR**: Optical Character Recognition library for detecting text in images.
- **YoloV8**: For detecting vehicles in videos.
- **OpenCV**: Used for image preprocessing (grayscale conversion, contrast adjustment).
- **Python Regular Expressions**: For filtering valid license plates and provinces.

## Requirements
- Python 3.x
- EasyOCR
- YoloV8
- OpenCV
- Other datasets on Roboflow

## How to Use

Prepare your images of vehicle license plates and place them in the designated folders `l-2/train/images` and `license-plate-1/train/images`

1. Run the script to detect license plates and provinces:

```bash
python detect_license_plates.py
```

2. For video detection using YoloV8, run:

```bash
python detect_vehicles_in_video.py
```
The script will output the detected license plates, provinces, and vehicle detection results along with the corresponding filenames or video frames.

## Example Output

```yaml
--- Organized Data ---
File: 0-70981418_2287209791405960_8660685034216226816_n_jpg.rf.056238bb14a6d42efb80c4d5765b35e7.jpg
  License Plate: 6กต9651
  Province: กรุงเทพมหานคร
File: 0-243988384_4205820349544885_4293639815547626321_n_jpg.rf.378f3a47d315ae9f5fe379627d28e75b.jpg
  License Plate: 2กฉ6408
  Province: กรุงเทพมหานคร
...
```
## Floder Structure

```bash
├── l-2
│   └── train
│       └── images
├── license-plate-1
│   └── train
│       └── images
├── detect_license_plates.py    # Main detection script for images
├── detect_vehicles_in_video.py # Script for detecting vehicles in videos using YoloV8
├── requirements.txt            # List of dependencies
└── README.md                   # Project documentation
```
## Contributing
Feel free to fork the repository, open issues, and submit pull requests for bug fixes, feature requests, or improvements.

## License
This project is licensed under the MIT License - see the [LICENSE DATASETS](https://universe.roboflow.com/search?q=thai%2520license-plate) Roboflow for more details.

## Acknowledgments
- EasyOCR library for text extraction.
- YoloV8 for vehicle detection in videos.
- OpenCV for image processing.
- Thanks to the contributors and open-source community.

## Manual
[โครงงาน นวัตกรรมอิเล็กทรอนิกส์อัจฉริยะ และ IOT final](https://drive.google.com/file/d/1ejc72Lrex97TD1GRQhnJGB0QtPqFC12G/view?usp=drive_link)
