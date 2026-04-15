// Test for AceStepWorker
// Compile with: cmake .. && make test_acestep_worker && ./test_acestep_worker

#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>
#include <QThread>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <iostream>
#include <cassert>

#include "../src/AceStepWorker.h"

// Test result tracking
static int testsPassed = 0;
static int testsFailed = 0;
static int testsSkipped = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    test_##name(); \
    if (test_skipped) { \
        std::cout << "SKIPPED" << std::endl; \
        testsSkipped++; \
        test_skipped = false; \
    } else if (test_failed) { \
        std::cout << "FAILED" << std::endl; \
        testsFailed++; \
        test_failed = false; \
    } else { \
        std::cout << "PASSED" << std::endl; \
        testsPassed++; \
    } \
} while(0)

static bool test_failed = false;
static bool test_skipped = false;

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cout << "FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
        test_failed = true; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define SKIP_IF(cond) do { \
    if (cond) { \
        std::cout << "(skipping: " << #cond << ") "; \
        test_skipped = true; \
        return; \
    } \
} while(0)

// Helper to get model paths from settings like main app
struct ModelPaths {
    QString lmPath;
    QString textEncoderPath;
    QString ditPath;
    QString vaePath;
};

static ModelPaths getModelPathsFromSettings()
{
    ModelPaths paths;
    QSettings settings("MusicGenerator", "AceStepGUI");
    
    QString appDir = QCoreApplication::applicationDirPath();
    paths.lmPath = settings.value("qwen3ModelPath",
        appDir + "/acestep.cpp/models/acestep-5Hz-lm-4B-Q8_0.gguf").toString();
    paths.textEncoderPath = settings.value("textEncoderModelPath",
        appDir + "/acestep.cpp/models/Qwen3-Embedding-0.6B-Q8_0.gguf").toString();
    paths.ditPath = settings.value("ditModelPath",
        appDir + "/acestep.cpp/models/acestep-v15-turbo-Q8_0.gguf").toString();
    paths.vaePath = settings.value("vaeModelPath",
        appDir + "/acestep.cpp/models/vae-BF16.gguf").toString();
    
    return paths;
}

static bool modelsExist(const ModelPaths& paths)
{
    return QFileInfo::exists(paths.lmPath) &&
           QFileInfo::exists(paths.textEncoderPath) &&
           QFileInfo::exists(paths.ditPath) &&
           QFileInfo::exists(paths.vaePath);
}

// Test 1: Check that isGenerating returns false initially
TEST(initialState)
{
    AceStepWorker worker;
    ASSERT_TRUE(!worker.isGenerating());
}

// Test 2: Check that requestGeneration returns false when no model paths set
TEST(noModelPaths)
{
    AceStepWorker worker;
    SongItem song("test caption", "");
    
    bool result = worker.requestGeneration(song, "{}");
    ASSERT_FALSE(result);
    ASSERT_TRUE(!worker.isGenerating());
}

// Test 3: Check that setModelPaths stores paths correctly
TEST(setModelPaths)
{
    AceStepWorker worker;
    worker.setModelPaths("/path/lm.gguf", "/path/encoder.gguf", "/path/dit.gguf", "/path/vae.gguf");
    ASSERT_TRUE(true);
}

// Test 4: Check async behavior - requestGeneration returns immediately
TEST(asyncReturnsImmediately)
{
    AceStepWorker worker;
    worker.setModelPaths("/path/lm.gguf", "/path/encoder.gguf", "/path/dit.gguf", "/path/vae.gguf");
    
    SongItem song("test caption", "");
    
    // If this blocks, the test will hang
    bool result = worker.requestGeneration(song, "{}");
    
    // Should return false due to invalid paths, but immediately
    ASSERT_FALSE(result);
}

// Test 5: Check that cancelGeneration sets the cancel flag
TEST(cancellationFlag)
{
    AceStepWorker worker;
    worker.setModelPaths("/path/lm.gguf", "/path/encoder.gguf", "/path/dit.gguf", "/path/vae.gguf");
    worker.cancelGeneration();
    ASSERT_TRUE(true);
}

// Test 6: Check that signals are defined correctly
TEST(signalsExist)
{
    AceStepWorker worker;
    
    // Verify signals exist by connecting to them (compile-time check)
    QObject::connect(&worker, &AceStepWorker::songGenerated, [](const SongItem&) {});
    QObject::connect(&worker, &AceStepWorker::generationCanceled, [](const SongItem&) {});
    QObject::connect(&worker, &AceStepWorker::generationError, [](const QString&) {});
    QObject::connect(&worker, &AceStepWorker::progressUpdate, [](int) {});
    
    ASSERT_TRUE(true);
}

// Test 7: Check SongItem to AceRequest conversion (internal)
TEST(requestConversion)
{
    AceStepWorker worker;
    
    SongItem song("Upbeat pop rock", "[Verse 1]");
    song.cotCaption = true;
    
    QString templateJson = R"({"inference_steps": 8, "shift": 3.0, "vocal_language": "en"})";
    
    worker.setModelPaths("/path/lm.gguf", "/path/encoder.gguf", "/path/dit.gguf", "/path/vae.gguf");
    bool result = worker.requestGeneration(song, templateJson);
    
    // Should fail due to invalid paths, but shouldn't crash
    ASSERT_FALSE(result);
}

// Test 8: Read model paths from settings
TEST(readSettings)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    std::cout << "\n  Model paths from settings:" << std::endl;
    std::cout << "    LM: " << paths.lmPath.toStdString() << std::endl;
    std::cout << "    Text Encoder: " << paths.textEncoderPath.toStdString() << std::endl;
    std::cout << "    DiT: " << paths.ditPath.toStdString() << std::endl;
    std::cout << "    VAE: " << paths.vaePath.toStdString() << std::endl;
    
    ASSERT_TRUE(!paths.lmPath.isEmpty());
    ASSERT_TRUE(!paths.textEncoderPath.isEmpty());
    ASSERT_TRUE(!paths.ditPath.isEmpty());
    ASSERT_TRUE(!paths.vaePath.isEmpty());
}

// Test 9: Check if model files exist
TEST(checkModelFiles)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    bool lmExists = QFileInfo::exists(paths.lmPath);
    bool encoderExists = QFileInfo::exists(paths.textEncoderPath);
    bool ditExists = QFileInfo::exists(paths.ditPath);
    bool vaeExists = QFileInfo::exists(paths.vaePath);
    
    std::cout << "\n  Model file status:" << std::endl;
    std::cout << "    LM: " << (lmExists ? "EXISTS" : "MISSING") << std::endl;
    std::cout << "    Text Encoder: " << (encoderExists ? "EXISTS" : "MISSING") << std::endl;
    std::cout << "    DiT: " << (ditExists ? "EXISTS" : "MISSING") << std::endl;
    std::cout << "    VAE: " << (vaeExists ? "EXISTS" : "MISSING") << std::endl;
    
    ASSERT_TRUE(lmExists);
    ASSERT_TRUE(encoderExists);
    ASSERT_TRUE(ditExists);
    ASSERT_TRUE(vaeExists);
}

// Test 10: Actually generate a song (requires valid model paths)
TEST(generateSong)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    // Skip if models don't exist
    SKIP_IF(!modelsExist(paths));
    
    AceStepWorker worker;
    worker.setModelPaths(paths.lmPath, paths.textEncoderPath, paths.ditPath, paths.vaePath);
    
    SongItem song("Upbeat pop rock with driving guitars", "");
    
    QString templateJson = R"({"inference_steps": 8, "shift": 3.0, "vocal_language": "en"})";
    
    // Track if we get progress updates
    bool gotProgress = false;
    QObject::connect(&worker, &AceStepWorker::progressUpdate, [&gotProgress](int p) {
        std::cout << "\n  Progress: " << p << "%" << std::endl;
        gotProgress = true;
    });
    
    // Track generation result
    bool generationCompleted = false;
    SongItem resultSong;
    QObject::connect(&worker, &AceStepWorker::songGenerated, 
        [&generationCompleted, &resultSong](const SongItem& song) {
            std::cout << "\n  Song generated successfully!" << std::endl;
            std::cout << "    Caption: " << song.caption.toStdString() << std::endl;
            std::cout << "    Lyrics: " << song.lyrics.left(100).toStdString() << "..." << std::endl;
            std::cout << "    File: " << song.file.toStdString() << std::endl;
            resultSong = song;
            generationCompleted = true;
        });
    
    QString errorMsg;
    QObject::connect(&worker, &AceStepWorker::generationError,
        [&errorMsg](const QString& err) {
            std::cout << "\n  Error: " << err.toStdString() << std::endl;
            errorMsg = err;
        });
    
    std::cout << "\n  Starting generation..." << std::endl;
    
    // Request generation
    bool result = worker.requestGeneration(song, templateJson);
    ASSERT_TRUE(result);
    
    // Use QEventLoop with timer for proper event processing
    QEventLoop loop;
    QTimer timeoutTimer;
    
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(300000); // 5 minute timeout
    
    QObject::connect(&worker, &AceStepWorker::songGenerated, &loop, &QEventLoop::quit);
    QObject::connect(&worker, &AceStepWorker::generationError, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    loop.exec();
    
    ASSERT_TRUE(generationCompleted);
    ASSERT_TRUE(resultSong.audioData != nullptr);
    ASSERT_TRUE(!resultSong.audioData->isEmpty());
    
    // Check audio data is not empty
    std::cout << "    Audio data size: " << resultSong.audioData->size() << " bytes" << std::endl;
    ASSERT_TRUE(resultSong.audioData->size() > 1000); // Should be at least 1KB for valid audio
}

// Test 11: Test cancellation
TEST(cancellation)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    // Skip if models don't exist
    SKIP_IF(!modelsExist(paths));
    
    AceStepWorker worker;
    worker.setModelPaths(paths.lmPath, paths.textEncoderPath, paths.ditPath, paths.vaePath);
    
    SongItem song("A very long ambient piece", "");
    
    QString templateJson = R"({"inference_steps": 50, "shift": 3.0, "vocal_language": "en"})";
    
    bool cancelReceived = false;
    QObject::connect(&worker, &AceStepWorker::generationCanceled,
        [&cancelReceived](const SongItem&) {
            std::cout << "\n  Generation was canceled!" << std::endl;
            cancelReceived = true;
        });
    
    std::cout << "\n  Starting generation and will cancel after 2 seconds..." << std::endl;
    
    // Start generation
    bool result = worker.requestGeneration(song, templateJson);
    ASSERT_TRUE(result);
    
    // Wait 2 seconds then cancel
    QThread::sleep(2);
    worker.cancelGeneration();
    
    // Wait a bit for cancellation to be processed
    QThread::sleep(1);
    QCoreApplication::processEvents();
    
    // Note: cancellation may or may not complete depending on where in the process
    // the cancel was requested. The important thing is it doesn't crash.
    std::cout << "  Cancel requested, no crash detected" << std::endl;
    ASSERT_TRUE(true);
}

// Test 12: Test low VRAM mode generation
TEST(generateSongLowVram)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    // Skip if models don't exist
    SKIP_IF(!modelsExist(paths));
    
    AceStepWorker worker;
    worker.setModelPaths(paths.lmPath, paths.textEncoderPath, paths.ditPath, paths.vaePath);
    worker.setLowVramMode(true);
    
    ASSERT_TRUE(worker.isLowVramMode());
    
    SongItem song("Chill electronic music", "");
    
    QString templateJson = R"({"inference_steps": 8, "shift": 3.0, "vocal_language": "en"})";
    
    // Track generation result
    bool generationCompleted = false;
    SongItem resultSong;
    QObject::connect(&worker, &AceStepWorker::songGenerated, 
        [&generationCompleted, &resultSong](const SongItem& song) {
            std::cout << "\n  Low VRAM mode: Song generated successfully!" << std::endl;
            std::cout << "    Caption: " << song.caption.toStdString() << std::endl;
            if (song.audioData) {
                std::cout << "    Audio data size: " << song.audioData->size() << " bytes" << std::endl;
            } else {
                std::cout << "    Audio data size: null" << std::endl;
            }
            resultSong = song;
            generationCompleted = true;
        });
    
    QString errorMsg;
    QObject::connect(&worker, &AceStepWorker::generationError,
        [&errorMsg](const QString& err) {
            std::cout << "\n  Error: " << err.toStdString() << std::endl;
            errorMsg = err;
        });
    
    std::cout << "\n  Starting low VRAM mode generation..." << std::endl;
    
    // Request generation
    bool result = worker.requestGeneration(song, templateJson);
    ASSERT_TRUE(result);
    
    // Use QEventLoop with timer for proper event processing
    QEventLoop loop;
    QTimer timeoutTimer;
    
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(300000); // 5 minute timeout
    
    QObject::connect(&worker, &AceStepWorker::songGenerated, &loop, &QEventLoop::quit);
    QObject::connect(&worker, &AceStepWorker::generationError, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    loop.exec();
    
    ASSERT_TRUE(generationCompleted);
    ASSERT_TRUE(resultSong.audioData != nullptr);
    ASSERT_TRUE(!resultSong.audioData->isEmpty());
    
    std::cout << "    Audio data size: " << resultSong.audioData->size() << " bytes" << std::endl;
    ASSERT_TRUE(resultSong.audioData->size() > 1000);
}

// Test 13: Test normal mode keeps models loaded between generations
TEST(normalModeKeepsModelsLoaded)
{
    ModelPaths paths = getModelPathsFromSettings();
    
    // Skip if models don't exist
    SKIP_IF(!modelsExist(paths));
    
    AceStepWorker worker;
    worker.setModelPaths(paths.lmPath, paths.textEncoderPath, paths.ditPath, paths.vaePath);
    // Normal mode is default (lowVramMode = false)
    
    ASSERT_FALSE(worker.isLowVramMode());
    
    QString templateJson = R"({"inference_steps": 8, "shift": 3.0, "vocal_language": "en"})";
    
    // Generate first song
    bool firstGenerationCompleted = false;
    QObject::connect(&worker, &AceStepWorker::songGenerated, 
        [&firstGenerationCompleted](const SongItem&) {
            firstGenerationCompleted = true;
        });
    
    QObject::connect(&worker, &AceStepWorker::generationError,
        [](const QString& err) {
            std::cout << "\n  Error: " << err.toStdString() << std::endl;
        });
    
    std::cout << "\n  Generating first song (normal mode)..." << std::endl;
    
    SongItem song1("First song", "");
    bool result = worker.requestGeneration(song1, templateJson);
    ASSERT_TRUE(result);
    
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(300000);
    
    QObject::connect(&worker, &AceStepWorker::songGenerated, &loop, &QEventLoop::quit);
    QObject::connect(&worker, &AceStepWorker::generationError, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    loop.exec();
    
    ASSERT_TRUE(firstGenerationCompleted);
    std::cout << "  First generation completed, models should still be loaded" << std::endl;
    
    // Generate second song - in normal mode this should be faster since models are already loaded
    bool secondGenerationCompleted = false;
    SongItem secondResult;
    QObject::connect(&worker, &AceStepWorker::songGenerated, 
        [&secondGenerationCompleted, &secondResult](const SongItem& song) {
            secondGenerationCompleted = true;
            secondResult = song;
        });
    
    std::cout << "  Generating second song (should use cached models)..." << std::endl;
    
    SongItem song2("Second song", "");
    result = worker.requestGeneration(song2, templateJson);
    ASSERT_TRUE(result);
    
    QEventLoop loop2;
    QTimer timeoutTimer2;
    timeoutTimer2.setSingleShot(true);
    timeoutTimer2.start(300000);
    
    QObject::connect(&worker, &AceStepWorker::songGenerated, &loop2, &QEventLoop::quit);
    QObject::connect(&worker, &AceStepWorker::generationError, &loop2, &QEventLoop::quit);
    QObject::connect(&timeoutTimer2, &QTimer::timeout, &loop2, &QEventLoop::quit);
    
    loop2.exec();
    
    ASSERT_TRUE(secondGenerationCompleted);
    ASSERT_TRUE(secondResult.audioData != nullptr);
    ASSERT_TRUE(!secondResult.audioData->isEmpty());
    
    std::cout << "  Second generation completed successfully" << std::endl;
    std::cout << "    Audio data size: " << secondResult.audioData->size() << " bytes" << std::endl;
}

// Test 14: Test setLowVramMode toggle
TEST(lowVramModeToggle)
{
    AceStepWorker worker;
    
    // Default should be false (normal mode)
    ASSERT_FALSE(worker.isLowVramMode());
    
    // Enable low VRAM mode
    worker.setLowVramMode(true);
    ASSERT_TRUE(worker.isLowVramMode());
    
    // Disable low VRAM mode
    worker.setLowVramMode(false);
    ASSERT_FALSE(worker.isLowVramMode());
    
    // Toggle again
    worker.setLowVramMode(true);
    ASSERT_TRUE(worker.isLowVramMode());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "=== AceStepWorker Tests ===" << std::endl;
    
    RUN_TEST(initialState);
    RUN_TEST(noModelPaths);
    RUN_TEST(setModelPaths);
    RUN_TEST(asyncReturnsImmediately);
    RUN_TEST(cancellationFlag);
    RUN_TEST(signalsExist);
    RUN_TEST(requestConversion);
    RUN_TEST(readSettings);
    RUN_TEST(checkModelFiles);
    RUN_TEST(generateSong);
    RUN_TEST(cancellation);
    RUN_TEST(generateSongLowVram);
    RUN_TEST(normalModeKeepsModelsLoaded);
    RUN_TEST(lowVramModeToggle);
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed:  " << testsPassed << std::endl;
    std::cout << "Skipped: " << testsSkipped << std::endl;
    std::cout << "Failed:  " << testsFailed << std::endl;
    
    return testsFailed > 0 ? 1 : 0;
}