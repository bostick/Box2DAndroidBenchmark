
// SPDX-FileCopyrightText: 2024 Erin Catto
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#include "TaskScheduler_c.h"

#include "box2d/box2d.h"
#include "box2d/math_functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined( _WIN64 )
#include <windows.h>
#elif defined( __APPLE__ )
#include <unistd.h>
#elif defined( __linux__ )
#include <unistd.h>
#endif

#include <jni.h>

#include <android/log.h>

char filesDir[200];


#define TAG "Box2DBenchmark"

#define ARRAY_COUNT( A ) (int)( sizeof( A ) / sizeof( A[0] ) )
#define MAYBE_UNUSED( x ) ( (void)( x ) )

typedef b2WorldId CreateBenchmarkFcn( b2WorldDef* worldDef );
extern b2WorldId JointGrid( b2WorldDef* worldDef );
extern b2WorldId LargePyramid( b2WorldDef* worldDef );
extern b2WorldId ManyPyramids( b2WorldDef* worldDef );
extern b2WorldId Smash( b2WorldDef* worldDef );
extern b2WorldId Tumbler( b2WorldDef* worldDef );

typedef struct Benchmark
{
    const char* name;
    CreateBenchmarkFcn* createFcn;
    int stepCount;
} Benchmark;

#define MAX_TASKS 128
#define THREAD_LIMIT 32

typedef struct TaskData
{
    b2TaskCallback* box2dTask;
    void* box2dContext;
} TaskData;

enkiTaskScheduler* scheduler;
enkiTaskSet* tasks[MAX_TASKS];
TaskData taskData[MAX_TASKS];
int taskCount;

int GetNumberOfCores()
{
#if defined( _WIN64 )
    SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return sysinfo.dwNumberOfProcessors;
#elif defined( __APPLE__ )
    return (int)sysconf( _SC_NPROCESSORS_ONLN );
#elif defined( __linux__ )
    return (int)sysconf( _SC_NPROCESSORS_ONLN );
#else
    return 1;
#endif
}

void ExecuteRangeTask( uint32_t start, uint32_t end, uint32_t threadIndex, void* context )
{
    TaskData* data = context;
    data->box2dTask( start, end, threadIndex, data->box2dContext );
}

static void* EnqueueTask( b2TaskCallback* box2dTask, int itemCount, int minRange, void* box2dContext, void* userContext )
{
    MAYBE_UNUSED( userContext );

    if ( taskCount < MAX_TASKS )
    {
        enkiTaskSet* task = tasks[taskCount];
        TaskData* data = taskData + taskCount;
        data->box2dTask = box2dTask;
        data->box2dContext = box2dContext;

        struct enkiParamsTaskSet params;
        params.minRange = minRange;
        params.setSize = itemCount;
        params.pArgs = data;
        params.priority = 0;

        enkiSetParamsTaskSet( task, params );
        enkiAddTaskSet( scheduler, task );

        ++taskCount;

        return task;
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, TAG, "MAX_TASKS exceeded!!!" );
        box2dTask( 0, itemCount, 0, box2dContext );
        return NULL;
    }
}

static void FinishTask( void* userTask, void* userContext )
{
    MAYBE_UNUSED( userContext );

    enkiTaskSet* task = userTask;
    enkiWaitForTaskSet( scheduler, task );
}

// Box2D benchmark application. On Windows I recommend running this in an administrator command prompt. Don't use Windows Terminal.
int main( int argc, char** argv )
{
    Benchmark benchmarks[] = {
        { "joint_grid", JointGrid, 500 },
        { "large_pyramid", LargePyramid, 500 },
        { "many_pyramids", ManyPyramids, 200 },
        { "smash", Smash, 300 },
        { "tumbler", Tumbler, 750 },
    };

    int benchmarkCount = ARRAY_COUNT( benchmarks );

    int maxThreadCount = GetNumberOfCores();
    int runCount = 4;
    int singleBenchmark = -1;
    int singleWorkerCount = -1;
    b2Counters counters = { 0 };
    bool enableContinuous = true;

    assert( maxThreadCount <= THREAD_LIMIT );

    for ( int i = 1; i < argc; ++i )
    {
        const char* arg = argv[i];
        if ( strncmp( arg, "-t=", 3 ) == 0 )
        {
            int threadCount = atoi( arg + 3 );
            maxThreadCount = b2ClampInt( threadCount, 1, maxThreadCount );
        }
        else if ( strncmp( arg, "-b=", 3 ) == 0 )
        {
            singleBenchmark = atoi( arg + 3 );
            singleBenchmark = b2ClampInt( singleBenchmark, 0, benchmarkCount - 1 );
        }
        else if ( strncmp( arg, "-w=", 3 ) == 0 )
        {
            singleWorkerCount = atoi( arg + 3 );
        }
        else if ( strncmp( arg, "-r=", 3 ) == 0 )
        {
            runCount = b2ClampInt(atoi( arg + 3 ), 1, 16);
        }
        else if ( strcmp( arg, "-h" ) == 0 )
        {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Usage\n"
                    "-t=<integer>: the maximum number of threads to use\n"
                    "-b=<integer>: run a single benchmark\n"
                    "-w=<integer>: run a single worker count\n"
                    "-r=<integer>: number of repeats (default is 4)\n" );
            exit( 0 );
        }
    }

    if ( singleWorkerCount != -1 )
    {
        singleWorkerCount = b2ClampInt( singleWorkerCount, 1, maxThreadCount );
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "Starting Box2D benchmarks" );
    __android_log_print(ANDROID_LOG_INFO, TAG, "======================================" );

    for ( int benchmarkIndex = 0; benchmarkIndex < benchmarkCount; ++benchmarkIndex )
    {
        if ( singleBenchmark != -1 && benchmarkIndex != singleBenchmark )
        {
            continue;
        }

#ifdef NDEBUG
        int stepCount = benchmarks[benchmarkIndex].stepCount;
#else
        int stepCount = 10;
#endif

        bool countersAcquired = false;

        __android_log_print(ANDROID_LOG_INFO, TAG, "benchmark: %s, steps = %d", benchmarks[benchmarkIndex].name, stepCount );

        float maxFps[THREAD_LIMIT] = { 0 };

        for ( int threadCount = 1; threadCount <= maxThreadCount; ++threadCount )
        {
            if ( singleWorkerCount != -1 && singleWorkerCount != threadCount )
            {
                continue;
            }

            __android_log_print(ANDROID_LOG_INFO, TAG, "thread count: %d", threadCount );

            for ( int runIndex = 0; runIndex < runCount; ++runIndex )
            {
                scheduler = enkiNewTaskScheduler();
                struct enkiTaskSchedulerConfig config = enkiGetTaskSchedulerConfig( scheduler );
                config.numTaskThreadsToCreate = threadCount - 1;
                enkiInitTaskSchedulerWithConfig( scheduler, config );

                for ( int taskIndex = 0; taskIndex < MAX_TASKS; ++taskIndex )
                {
                    tasks[taskIndex] = enkiCreateTaskSet( scheduler, ExecuteRangeTask );
                }

                b2WorldDef worldDef = b2DefaultWorldDef();
                worldDef.enableSleep = false;
                worldDef.enableContinuous = enableContinuous;
                worldDef.enqueueTask = EnqueueTask;
                worldDef.finishTask = FinishTask;
                worldDef.workerCount = threadCount;

                b2WorldId worldId = benchmarks[benchmarkIndex].createFcn( &worldDef );

                float timeStep = 1.0f / 60.0f;
                int subStepCount = 4;

                // Initial step can be expensive and skew benchmark
                b2World_Step( worldId, timeStep, subStepCount );
                taskCount = 0;

                b2Timer timer = b2CreateTimer();

                for ( int step = 0; step < stepCount; ++step )
                {
                    b2World_Step( worldId, timeStep, subStepCount );
                    taskCount = 0;
                }

                float ms = b2GetMilliseconds( &timer );
                float fps = 1000.0f * stepCount / ms;
                __android_log_print(ANDROID_LOG_INFO, TAG, "run %d : %g (ms), %g (fps)", runIndex, ms, fps );

                maxFps[threadCount - 1] = b2MaxFloat( maxFps[threadCount - 1], fps );

                if ( countersAcquired == false )
                {
                    counters = b2World_GetCounters( worldId );
                    countersAcquired = true;
                }

                b2DestroyWorld( worldId );

                for ( int taskIndex = 0; taskIndex < MAX_TASKS; ++taskIndex )
                {
                    enkiDeleteTaskSet( scheduler, tasks[taskIndex] );
                    tasks[taskIndex] = NULL;
                    taskData[taskIndex] = ( TaskData ){ 0 };
                }

                enkiDeleteTaskScheduler( scheduler );
                scheduler = NULL;
            }
        }

        __android_log_print(ANDROID_LOG_INFO, TAG, "body %d / shape %d / contact %d / joint %d / stack %d", counters.bodyCount, counters.shapeCount,
                counters.contactCount, counters.jointCount, counters.stackUsed );

        char fileName[200] = { 0 };
        int res = snprintf( fileName, sizeof(fileName), "%s/%s.csv", filesDir, benchmarks[benchmarkIndex].name );
        if (res < 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "snprintf failed" );
            exit(1);
        }
        if (res >= sizeof(fileName)) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "snprintf failed" );
            exit(1);
        }
        FILE* file = fopen( fileName, "w" );
        if ( file == NULL )
        {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "opening %s failed", fileName );
            exit(1);
        }

        res = fprintf( file, "threads,fps" );
        if (res < 0) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "fprintf failed" );
            exit(1);
        }
        for ( int threadIndex = 1; threadIndex <= maxThreadCount; ++threadIndex )
        {
            res = fprintf( file, "%d,%g", threadIndex, maxFps[threadIndex - 1] );
            if (res < 0) {
                __android_log_print(ANDROID_LOG_ERROR, TAG, "fprintf failed" );
                exit(1);
            }
        }

        fclose( file );
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "======================================" );
    __android_log_print(ANDROID_LOG_INFO, TAG, "All Box2D benchmarks complete!" );

    return 0;
}



extern JNIEXPORT jint JNICALL Java_com_brentonbostick_box2dbenchmark_NativeLoader_setFilesDir(JNIEnv *env, jobject jobj, jstring jfilesDir) {

    (void)jobj;

    const char *tmp = (*env)->GetStringUTFChars(env, jfilesDir, NULL);
    if (tmp == NULL) {

        __android_log_print(ANDROID_LOG_ERROR, TAG, "GetStringUTFChars failed");

        return 1;
    }

    strncpy(filesDir, tmp, sizeof(filesDir));
    if (filesDir[sizeof(filesDir) - 1] != '\0') {

        __android_log_print(ANDROID_LOG_ERROR, TAG, "filesDir is truncated");

        (*env)->ReleaseStringUTFChars(env, jfilesDir, tmp);

        return 1;
    }

    (*env)->ReleaseStringUTFChars(env, jfilesDir, tmp);

    return 0;
}


