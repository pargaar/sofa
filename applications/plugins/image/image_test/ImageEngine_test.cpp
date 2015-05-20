/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 RC 1        *
*                (c) 2006-2011 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU General Public License as published by the Free  *
* Software Foundation; either version 2 of the License, or (at your option)   *
* any later version.                                                          *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
* more details.                                                               *
*                                                                             *
* You should have received a copy of the GNU General Public License along     *
* with this program; if not, write to the Free Software Foundation, Inc., 51  *
* Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.                   *
*******************************************************************************
*                            SOFA :: Applications                             *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#include <sofa/core/objectmodel/Data.h>
#include <plugins/SceneCreator/SceneCreator.h>
//Including Simulation
#include <SofaComponentMain/init.h>
#include <sofa/simulation/graph/DAGSimulation.h>

#include <plugins/SofaTest/Sofa_test.h>
#include <plugins/image/ImageContainer.h>
#include <plugins/image/ImageViewer.h>
#include <plugins/image/TestImageEngine.h>

namespace sofa {

/**  Test suite for engine data image.
 * Create a simple scene with an engine which computes an output image from an input image at each time step.
 * Visualize the output image of the engine with ImageViewer.
 * The input image of ImageViewer is then linked to the ouput image of the engine.
 * Copy on Write option is true.
 * When we launch the simulation at the end during destruction there is a segmentation fault,
 * because the buffer data of the ouput image of the Engine is shared with the buffer of the image of image viewer.
 * Note: the function draw of ImageViewer is acutally not called in this test (it works with the gui).
  */
struct ImageEngine_test : public Sofa_test<>
{

    // Root of the scene graph
    simulation::Node::SPtr root;


    // Unload scene
    void TearDown()
    {
//        std::cerr << "start TearDown" << std::endl;
        if (root!=NULL)
            sofa::simulation::getSimulation()->unload(root);
//        std::cerr << "end TearDown" << std::endl;
    }




    // Test link
    void testDataLink()
    {
        typedef defaulttype::Image<unsigned char> Image;

        core::objectmodel::Data< Image > data1;
        core::objectmodel::Data< Image > data2;


        // Set data1
        Image::CImgT img( (std::string(IMAGETEST_SCENES_DIR) + "/lena.jpg").c_str() );
//        img.display("loaded image");
        data1.setValue(img);
//        data1.getValue().getCImg().display("data1");


        // Set data link
        sofa::modeling::setDataLink(&data1,&data2);
        data1.getValue();

//        data2.getValue().getCImg().display("data2");


        // Check that data values are the same
        ASSERT_EQ(data1.getValue(),data2.getValue());

        // Check if pointers are equal
        if(&data1.getValue()!= &data2.getValue())
        {
            ADD_FAILURE() << "Data Link duplicates the datas ! " << std::endl;
        }

        // Change value of data1
        helper::WriteAccessor<Data< Image > > w1(data1);
        Image::CImgT outImg = w1->getCImg(0);
        outImg.fill(0);
//        data1.getValue().getCImg().display("data1 after clear");

        // Check that data values are the same
        ASSERT_EQ(data1.getValue(),data2.getValue());
//        data2.getValue().getCImg().display("data2 after clear");

        // Check if pointers are still equal
        if(&data1.getValue()!= &data2.getValue())
        {
            ADD_FAILURE() << "Data Link duplicates the datas ! " << std::endl;
        }
    }





    /// Scene with simple engines
    ///
    void testAnimate()
    {
        // simulation
        simulation::Simulation* simulation;

        // Image Container
        typedef sofa::component::container::ImageContainer< defaulttype::Image<unsigned char> > ImageContainer;
        ImageContainer::SPtr imageContainer;

        // Image Engine
        typedef sofa::component::engine::TestImageEngine< defaulttype::Image<unsigned char> > TestImageEngine;
        TestImageEngine::SPtr imageEngine;

        // Create a scene
        sofa::component::init();
        sofa::simulation::setSimulation(simulation = new sofa::simulation::graph::DAGSimulation());

        // Root node
        root = simulation->createNewGraph("root");

        // Input image
        // Image container
        imageContainer = sofa::modeling::addNew<ImageContainer>(root);

        // Set path to image for imageContainer
        std::string fileName = std::string(IMAGETEST_SCENES_DIR) + "/" + "beam.raw";
        imageContainer->m_filename.setValue(fileName);

        // ImageEngine
        imageEngine = sofa::modeling::addNew<TestImageEngine>(root);

        // Set data link: image of ImageContainer is input image of Engine.
        sofa::modeling::setDataLink(&imageContainer->image,&imageEngine->inputImage);

        // ImageEngine listening is true to update image at each time step
        imageEngine->f_listening.setValue(true);

        TestImageEngine::SPtr imageEngine2 = sofa::modeling::addNew<TestImageEngine>(root);
        sofa::modeling::setDataLink(&imageEngine->outputImage,&imageEngine2->inputImage);




        // Init simulation
        sofa::simulation::getSimulation()->init(root.get());

        //  do several steps of animation
        for(int l=0;l<2;++l)
        {
            sofa::simulation::getSimulation()->animate(root.get(),0.5);
//            std::cerr << "== loop "<<l<<std::endl;
//            std::cerr << "engine2 size " << imageEngine2->inputImage.getValue().getCImg(0).size() << std::endl;
//            std::cerr << "image shared shared: " << imageContainer->image.getValue().getCImg(0).is_shared() << " " << &imageContainer->image.getValue().getCImg(0)<< std::endl;
//            std::cerr << "engine input shared: " << imageEngine->inputImage.getValue().getCImg(0).is_shared() << " " << &imageEngine->inputImage.getValue().getCImg(0)<< std::endl;
//            std::cerr << "engine output shared: " << imageEngine->outputImage.getValue().getCImg(0).is_shared() << " " << &imageEngine->outputImage.getValue().getCImg(0)<< std::endl;
//            std::cerr << "engine2 input shared: " << imageEngine2->inputImage.getValue().getCImg(0).is_shared() << " " << &imageEngine2->inputImage.getValue().getCImg(0)<< std::endl;
        }


        // Check if pointers of images that should be shared are equal
        ASSERT_EQ(imageContainer->image.getValue(),imageEngine->inputImage.getValue());
        ASSERT_EQ(imageEngine->outputImage.getValue(),imageEngine2->inputImage.getValue());


    }



    /// Scene with an ImageViewer
    void testImageViewer()
    {
        // simulation
        simulation::Simulation* simulation;

        // Image Container
        typedef sofa::component::container::ImageContainer< defaulttype::Image<unsigned char> > ImageContainer;
        ImageContainer::SPtr imageContainer;

        // Image Engine
        typedef sofa::component::engine::TestImageEngine< defaulttype::Image<unsigned char> > TestImageEngine;
        TestImageEngine::SPtr imageEngine;

        // Image Viewer
        typedef sofa::component::misc::ImageViewer< defaulttype::Image<unsigned char> > ImageViewer;
        ImageViewer::SPtr imageViewer;

        // Create a scene
        sofa::component::init();
        sofa::simulation::setSimulation(simulation = new sofa::simulation::graph::DAGSimulation());

        // Root node
        root = simulation->createNewGraph("root");

        // Input image
        // Image container
        imageContainer = sofa::modeling::addNew<ImageContainer>(root);

        // Set path to image for imageContainer
        std::string fileName = std::string(IMAGETEST_SCENES_DIR) + "/" + "beam.raw";
        imageContainer->m_filename.setValue(fileName);

        // ImageEngine
        imageEngine = sofa::modeling::addNew<TestImageEngine>(root);

        // Set data link: image of ImageContainer is input image of Engine.
        sofa::modeling::setDataLink(&imageContainer->image,&imageEngine->inputImage);

        // ImageEngine listening is true to update image at each time step
        imageEngine->f_listening.setValue(true);


        //ImageViewer
        imageViewer = sofa::modeling::addNew<ImageViewer>(root);

        // Set data link: output image of engine is image of ImageViewer.
        sofa::modeling::setDataLink(&imageEngine->outputImage,&imageViewer->image);
        //sofa::modeling::setDataLink(&imageContainer->image,&imageViewer->image);



        // Init simulation
        sofa::simulation::getSimulation()->init(root.get());

        //  do several steps of animation
        for(int l=0;l<2;++l)
        {
            sofa::simulation::getSimulation()->animate(root.get(),0.5);
//            std::cerr << "== loop "<<l<<std::endl;
//            std::cerr << "viewer size " << imageViewer->image.getValue().getCImg(0).size() << std::endl;
//            std::cerr << "image shared shared: " << imageContainer->image.getValue().getCImg(0).is_shared() << " " << &imageContainer->image.getValue().getCImg(0)<< std::endl;
//            std::cerr << "engine input shared: " << imageEngine->inputImage.getValue().getCImg(0).is_shared() << " " << &imageEngine->inputImage.getValue().getCImg(0)<< std::endl;
//            std::cerr << "engine output shared: " << imageEngine->outputImage.getValue().getCImg(0).is_shared() << " " << &imageEngine->outputImage.getValue().getCImg(0)<< std::endl;
//            std::cerr << "viewer input shared: " << imageViewer->image.getValue().getCImg(0).is_shared() << " " << &imageViewer->image.getValue().getCImg(0)<< std::endl;
        }


        // Check if pointers of images that should be shared are equal
        ASSERT_EQ(imageContainer->image.getValue(),imageEngine->inputImage.getValue());
        ASSERT_EQ(imageEngine->outputImage.getValue(),imageViewer->image.getValue());

    }



};

// Test
TEST_F(ImageEngine_test , testDataLink )
{
    ASSERT_NO_THROW(this->testDataLink());
}

TEST_F(ImageEngine_test , testEngineDataLink )
{
    ASSERT_NO_THROW(this->testAnimate());
}

TEST_F(ImageEngine_test , testImageViewer )
{
    ASSERT_NO_THROW(this->testImageViewer());
}


}// namespace sofa


