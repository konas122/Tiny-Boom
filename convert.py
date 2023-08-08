import os
import cv2


def ppm2jpg(input_file):
    path = "./frames"
    input_file = os.path.join(path, input_file)
    img = cv2.imread(input_file, cv2.IMREAD_COLOR)
    os.remove(input_file)
    return img


if __name__ == "__main__":
    fps = 55          
    list = os.listdir("./frames")
    with open("./frames/" + list[0], encoding='ISO-8859-1') as f:
        line = f.readlines()[1]
    w, h = line.split(' ')
    size = (int(w), int(h))
    video = cv2.VideoWriter("output.avi", cv2.VideoWriter_fourcc('I', '4', '2', '0'), fps, size)
    for i in range(len(list)):
        video.write(ppm2jpg(list[i]))
    video.release()
    cv2.destroyAllWindows()
