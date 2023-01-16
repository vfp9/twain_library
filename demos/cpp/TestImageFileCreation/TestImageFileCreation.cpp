#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <dynarithmic/twain/twain_session.hpp> // for dynarithmic::twain::twain_session
#include <dynarithmic/twain/twain_source.hpp>  // for dynarithmic::twain::twain_source
#include <dynarithmic/twain/acquire_characteristics.hpp>  // for acquire_characteristics

void TestMultiOrSingleFile(std::string outDir, bool bTestSingle);

using namespace dynarithmic::twain;
struct Runner
{
    int Run(int argc, char* argv[]);
    ~Runner()
    {
        printf("\nPress Enter key to exit application...\n");
        char temp;
        std::cin.get(temp);
    }
};

// Global TWAIN session (not started here)
twain_session session;

int Runner::Run(int argc, char* argv[])
{
    int value = 0;
    if (argc < 3)
    {
        std::cout << "Usage: TestImageFileCreation test-to-run[1,2,3] output-directory\n\n";
        std::cout << "1 --> Single page files\n2 --> Multipage files\n3 --> Single and multipage files\n\n";
        std::cout << "Example:\n    TestImageFileCreation 1 c:\\saved_images";
        return 0;
    }

    try
    {
        value = std::stoi(argv[1]);
    }
    catch (std::exception& e)
    {
        std::cout << "Error: " << e.what();
        return -1;
    }

    std::string outDir = argv[2];
    if (outDir.back() != '\\')
        outDir.push_back('\\');

    // Set the application info for the session
    twain_session::twain_app_info appInfo;
    appInfo.set_product_name("TestImageFileCreation");
    session.set_app_info(appInfo);

    // start the session
    session.start();

    if (!session)
    {
        std::cout << "Could not start the twain session. \nReason: " << session.get_error_string(session.get_last_error());
        return -1;
    }

    switch (value)
    {
    case 1:
        TestMultiOrSingleFile(outDir, true);
        break;
    case 2:
        TestMultiOrSingleFile(outDir, false);
        break;
    default:
        TestMultiOrSingleFile(outDir, true);
        TestMultiOrSingleFile(outDir, false);
        break;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    Runner().Run(argc, argv);
}

void TestMultiOrSingleFile(std::string outDir, bool bTestSingle)
{
    // select a source
    auto selection = session.select_source();

    // check if user canceled the selection
    if (selection.canceled())
    {
        std::cout << "User canceled selecting the source\n";
        return;
    }

    // open the source from the selection above    
    twain_source Source(selection);
    auto ocr = DTWAIN_InitOCRInterface();
    std::vector<twain_session::supported_filetype_info> vectFileTypes;
    if (bTestSingle)
    {
        outDir += "Single\\";
        vectFileTypes = session.get_singlepage_filetype_info();
    }
    else
    {
        outDir += "Multi\\";
        vectFileTypes = session.get_multipage_filetype_info();
    }

    // write all the temporary files to the output directory
    session.set_temporary_directory(outDir);

    std::string filePrefix = outDir;

    // These are the black and white, 1 bit-per-pixel types
    std::set<filetype_value::value_type> sBlackWhite = { filetype_value::tiffgroup3multi,
                                                        filetype_value::tiffgroup4multi,
                                                        filetype_value::tiffgroup3,
                                                        filetype_value::tiffgroup4,
                                                        filetype_value::text,
                                                        filetype_value::textmulti };

    auto& ac = Source.get_acquire_characteristics();

    // Set the base file options for all file types
    auto& fc = ac.get_file_transfer_options().
                  enable_autocreate_directory();  // auto create the directory if it doesn't exist

    // Set the general options for all file types
    auto& gOpts = ac.get_general_options().
                     set_max_page_count(bTestSingle ? 1 : 2);  // set the max pages to acquire either 1 for single
                                                              // or 2 if multipage

    // turn off the user interface
    auto& uiOpts = ac.get_userinterface_options().show(false);

    // loop through all of the available file types
    for (auto& fileInfo : vectFileTypes)
    {
        // get the image type that will be saved
        auto fileType = fileInfo.get_type();

        // Create the base of the file name
        // we use the first extension of an image file can
        // have multiple extensions
        std::string extToUse = fileInfo.get_extensions().front();

        // now create the file name
        std::string fileName = filePrefix + fileInfo.get_name() + "." + extToUse;

        // Set the name and type
        fc.set_name(fileName).set_type(fileType);

        // Make sure pixel type is black/white for the corresponding
        // file type (TIFF G3, TIFF g4, etc.)
        color_value::value_type pt = color_value::default_color;
        if (sBlackWhite.count(fileType) )
            pt = color_value::bw;

        // Set the pixel type
        gOpts.set_pixeltype(pt);

        // Start the acquisition
        Source.acquire();

        // Output information
        std::cout << fileInfo.get_name() << " " << fileName << "\n";
    }
}
