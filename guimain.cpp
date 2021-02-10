// Dear ImGui: standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_glfw_opengl2/ folder**
// See imgui_impl_glfw.cpp for details.

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

volatile sig_atomic_t done = 0;

void sighandler(int sig)
{
    done = 1;
}

#include "imgui/imgui.h"
#include "backend/imgui_impl_glfw.h"
#include "backend/imgui_impl_opengl2.h"
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#include <jpeglib.h>

bool LoadTextureFromMem(const unsigned char *in_jpeg, ssize_t len, GLuint *out_texture, int *out_width, int *out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    /* More stuff */
    JSAMPARRAY buffer; /* Output row buffer */
    int row_stride;    /* physical row width in output buffer */

    /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    cinfo.err = jpeg_std_error(&jerr);
    /* Step 1: allocate and initialize JPEG decompression object */
    jpeg_create_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);

    /* Step 2: specify data source (eg, a file) */

    jpeg_mem_src(&cinfo, in_jpeg, len);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* Step 3: read file parameters with jpeg_read_header() */

    fprintf(stderr, "%s: %d %d\n", __func__, __LINE__, jpeg_read_header(&cinfo, TRUE));
    /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.txt for more info.
   */
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

    /* Step 5: Start decompressor */
    cinfo.out_color_space = JCS_GRAYSCALE;
    cinfo.scale_num = 480; // scale to 480p
    cinfo.scale_denom = cinfo.image_height;
    (void)jpeg_start_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    fprintf(stderr, "%s: output width: %d, output components: %d, row_stride: %d\n", __func__, cinfo.output_width, cinfo.output_components, row_stride);
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
    image_data = (unsigned char *)malloc(row_stride * cinfo.output_height);
    image_height = cinfo.output_height;
    image_width = cinfo.output_width;
    int loc = 0;
    fprintf(stderr, "%s: %d: Width = %d, Height = %d Size = %d\n", __func__, __LINE__, cinfo.output_width, cinfo.output_height, row_stride * cinfo.output_height);
    while (cinfo.output_scanline < cinfo.output_height)
    {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        memcpy(&(image_data[loc]), buffer[0], row_stride);
        loc += row_stride;
    }

    /* Step 7: Finish decompression */

    (void)jpeg_finish_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image_data);
    fprintf(stderr, "%s: %d %d\n", __func__, __LINE__, image_texture);
    free(image_data);
    *out_texture = image_texture;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    *out_width = image_width;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    *out_height = image_height;
    fprintf(stderr, "%s: %d\n", __func__, __LINE__);
    return true;
}

GLuint my_image_texture;
int my_image_width, my_image_height;

volatile bool conn_rdy = false;

typedef struct __attribute__((packed))
{
    unsigned width;
    unsigned height;
    float temp;
    uint64_t tstamp;
    int size;
} net_meta;

typedef struct
{
    net_meta *metadata;
    pthread_mutex_t lock;
    unsigned char *data;
} net_image;

net_image img;

static char rcv_buf[1024 * 1024 * 4];
void *rcv_thr(void *sock)
{
    img.metadata = (net_meta *)malloc(sizeof(net_meta));
    img.data = (unsigned char *)malloc(1024 * 1024 * 4);
    memset(rcv_buf, 0x0, sizeof(rcv_buf));
    while (!done)
    {
        fprintf(stderr, "%s: looping\n", __func__);
        fflush(stderr);
        memset(img.metadata, 0x0, sizeof(net_meta));
        memset(img.data, 0x0, 1024 * 1024 * 4);
        usleep(1000 * 1000 / 30); // receive at 120 Hz
        if (conn_rdy)
        {
            int32_t msg_sz = 0;
            int sz = read(*(int *)sock, &msg_sz, sizeof(uint32_t));
            if (sz <= 0)
                continue;
            if (sz == sizeof(uint32_t))
            {
                fprintf(stderr, "%s: Size = %d\n", __func__, msg_sz);
                fflush(stderr);
                if (msg_sz > 0 && msg_sz < 1024 * 1024 * 4)
                {
                    sz = recv(*(int *)sock, rcv_buf, msg_sz, MSG_WAITALL);
                }
                if (sz == msg_sz)
                {
                    // pthread_mutex_lock(&(img.lock));
                    memcpy(img.metadata, rcv_buf, sizeof(net_meta));
                    if (img.metadata->size > 0)
                    {
                        fprintf(stderr, "%s: Image Size = %d\n", __func__, img.metadata->size);
                        fflush(stderr);
                        memcpy(img.data, rcv_buf + sizeof(net_meta), img.metadata->size);
                        // pthread_mutex_unlock(&(img.lock));
                        LoadTextureFromMem(img.data, img.metadata->size, &my_image_texture, &my_image_width, &my_image_height);
                    }
                }
            }
        }
    }
    free(img.metadata);
    free(img.data);
    return NULL;
}

int main(int, char **)
{
    // setup signal handler
    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN); // so that client does not die when server does
    // set up client socket etc
    int sock = -1, valread;
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;

    // we will have to add a port

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Client Example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    ImFont *font = io.Fonts->AddFontFromFileTTF("./imgui/font/Roboto-Medium.ttf", 16.0f);
    if (font == NULL)
        io.Fonts->AddFontDefault();
    //IM_ASSERT(font != NULL);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool show_readout_win = true;

    static int jpg_qty = 70;

    pthread_t rcv_thread;
    int rc = pthread_create(&rcv_thread, NULL, rcv_thr, (void *)&sock);
    if (rc < 0)
    {
        fprintf(stderr, "main: Could not create receiver thread! Exiting...\n");
        goto end;
    }
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static int port = 12395;
            static char ipaddr[16] = "192.168.1.18";
            auto flag = ImGuiInputTextFlags_ReadOnly;
            if (!conn_rdy)
                flag = (ImGuiInputTextFlags_)0;

            ImGui::Begin("Connection Manager"); // Create a window called "Hello, world!" and append into it.

            ImGui::InputText("IP Address", ipaddr, sizeof(ipaddr), flag);
            ImGui::InputInt("Port", &port, 0, 0, flag);

            if (!conn_rdy || sock < 0)
            {
                if (ImGui::Button("Connect"))
                {
                    serv_addr.sin_port = htons(port);
                    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        printf("\n Socket creation error \n");
                        return -1;
                    }
                    if (inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr) <= 0)
                    {
                        printf("\nInvalid address/ Address not supported \n");
                    }
                    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                    {
                        printf("\nConnection Failed \n");
                    }
                    else
                        conn_rdy = true;
                }
            }
            else
            {
                if (ImGui::Button("Disconnect"))
                {
                    close(sock);
                    sock = -1;
                    conn_rdy = false;
                }
            }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (conn_rdy && sock > 0)
            {
                if (ImGui::InputInt("JPEG Quality", &jpg_qty, 1, 10))
                {
                    static char msg[1024];
                    int sz = snprintf(msg, 1024, "CMD_JPEG_SET_QUALITY%d", jpg_qty);
                    send(sock, msg, sz, 0);
                }
            }
            if (conn_rdy && sock > 0)
            {
                if (my_image_texture != NULL)
                {
                    ImGui::Text("pointer = %p", my_image_texture);
                    ImGui::Text("size = %d x %d", my_image_width, my_image_height);
                    ImGui::Image((void *)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
                }
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
        // you may need to backup/reset/restore current shader using the commented lines below.
        //GLint last_program;
        //glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        //glUseProgram(0);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        //glUseProgram(last_program);

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
end:
    done = 1;
    close(sock);
    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
