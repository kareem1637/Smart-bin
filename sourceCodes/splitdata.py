import os
import random
import shutil
# do not run this file more than once because it moves the image not copying it 
def take_random_sample(source_dir, dest_dir, sample_size):
    # Get all image files from the source directory
    all_images = [os.path.join(source_dir, file) for file in os.listdir(source_dir) if file.endswith(('.png', '.jpg', '.jpeg'))]
    
    # Randomly sample the specified number of images
    random_sample = random.sample(all_images, sample_size)
    
    # Create the destination folder if it doesn't exist
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
    
    # Move the sampled images to the destination folder
    for image in random_sample:
        shutil.move(image, dest_dir)
    
    print(f"Moved {sample_size} images from {source_dir} to {dest_dir}")

# Paths to the clean and dirty folders in the train dataset
train_clean_dir = 'D:/Smart bin/dataset/train/clean'
train_dirty_dir = 'D:/Smart bin/dataset/train/dirty'

# Paths to the clean and dirty folders in the test dataset
test_clean_dir = 'D:/Smart bin/dataset/val/clean'
test_dirty_dir = 'D:/Smart bin/dataset/val/dirty'

# Number of images to sample
sample_size = 150

# Move 100 images from 'clean' and 'dirty' folders to corresponding test folders
take_random_sample(train_clean_dir, test_clean_dir, sample_size)
take_random_sample(train_dirty_dir, test_dirty_dir, sample_size)
