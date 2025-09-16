/*
 * macOS native notifications implementation using UNUserNotificationCenter
 * 
 * This file provides a C interface to macOS's Objective-C UNUserNotificationCenter
 * framework, allowing Chatterino2's C++ code to send native macOS notifications.
 * 
 * Key features:
 * - Request and manage notification permissions
 * - Send notifications with title, body, and optional avatar image
 * - Handle user clicks on notifications to perform actions
 * - Support for sound/silent notifications based on user settings
 */

#include "MacOSNotifications.h"

#import <UserNotifications/UserNotifications.h>
#import <Foundation/Foundation.h>

#include <QString>
#include <QDebug>
#include <QFileInfo>
#include "common/QLogging.hpp"

Q_LOGGING_CATEGORY(chatterinoMacOSNotification, "chatterino.macos.notification");

// Forward declare the C function that will call back to C++
extern "C" void performReactionForMacOS(const char* channelName);

// Helper function to convert QString to NSString
@interface NSString (QString)
+ (NSString *)stringWithQString:(const QString &)qstring;
@end

@implementation NSString (QString)
+ (NSString *)stringWithQString:(const QString &)qstring {
    return [NSString stringWithUTF8String:qstring.toUtf8().constData()];
}
@end

// Notification delegate to handle user responses
// This delegate receives callbacks when users interact with notifications
@interface ChatterinoNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation ChatterinoNotificationDelegate

// Called when user clicks on a notification
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       didReceiveNotificationResponse:(UNNotificationResponse *)response
                withCompletionHandler:(void (^)(void))completionHandler {
    
    NSString *identifier = response.notification.request.identifier;
    
    // Extract channel name from identifier (format: "chatterino_channelname")
    if ([identifier hasPrefix:@"chatterino_"]) {
        NSString *channelName = [identifier substringFromIndex:[@"chatterino_" length]];
        
        qCDebug(chatterinoMacOSNotification) << "Notification clicked for channel:" 
            << QString::fromUtf8([channelName UTF8String]);
        
        // Call the C function that will handle the reaction
        performReactionForMacOS([channelName UTF8String]);
    }
    
    completionHandler();
}

@end

// Global variables
static BOOL g_permissionRequested = NO;
static ChatterinoNotificationDelegate *g_delegate = nil;

// C wrapper function implementations
extern "C" {

void chatterinoRequestNotificationPermission() {
    qCDebug(chatterinoMacOSNotification) << "chatterinoRequestNotificationPermission called";
    
    if (g_permissionRequested) {
        qCDebug(chatterinoMacOSNotification) << "Permission already requested, returning";
        return;
    }
    
    qCDebug(chatterinoMacOSNotification) << "Setting permission requested flag and proceeding";
    g_permissionRequested = YES;
    
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Set up delegate to handle notification responses
    if (!g_delegate) {
        g_delegate = [[ChatterinoNotificationDelegate alloc] init];
        center.delegate = g_delegate;
        qCDebug(chatterinoMacOSNotification) << "Set up notification delegate";
    }
    
    // Check current status first
    qCDebug(chatterinoMacOSNotification) << "Checking current notification settings";
    [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings * _Nonnull settings) {
        UNAuthorizationStatus currentStatus = settings.authorizationStatus;
        qCDebug(chatterinoMacOSNotification) << "Current authorization status:" << (int)currentStatus;
        
        if (currentStatus == UNAuthorizationStatusNotDetermined) {
            qCDebug(chatterinoMacOSNotification) << "Status is not determined, requesting permission";
            // If status is notDetermined, request permission
            UNAuthorizationOptions options = UNAuthorizationOptionAlert | UNAuthorizationOptionSound;
            
            [center requestAuthorizationWithOptions:options completionHandler:^(BOOL granted, NSError * _Nullable error) {
                if (error) {
                    qCWarning(chatterinoMacOSNotification) << "Notification permission error:" 
                        << QString::fromUtf8([error.localizedDescription UTF8String]);
                } else if (granted) {
                    qCDebug(chatterinoMacOSNotification) << "Notification permission granted";
                } else {
                    qCDebug(chatterinoMacOSNotification) << "Notification permission denied";
                }
            }];
        } else if (currentStatus == UNAuthorizationStatusAuthorized) {
            qCDebug(chatterinoMacOSNotification) << "Notification permission already granted";
        } else {
            qCDebug(chatterinoMacOSNotification) << "Notification permission denied or restricted";
        }
    }];
}

bool chatterinoHasNotificationPermission() {
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Use a semaphore to make this synchronous
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block BOOL hasPermission = NO;
    __block UNAuthorizationStatus authStatus = UNAuthorizationStatusNotDetermined;
    
    [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings * _Nonnull settings) {
        authStatus = settings.authorizationStatus;
        hasPermission = (authStatus == UNAuthorizationStatusAuthorized);
        dispatch_semaphore_signal(semaphore);
    }];
    
    // Wait for the async call to complete (with timeout)
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC); // 1 second timeout
    long result = dispatch_semaphore_wait(semaphore, timeout);
    
    qCDebug(chatterinoMacOSNotification) << "Permission check - Status:" << (int)authStatus 
        << "HasPermission:" << hasPermission << "Timeout:" << (result != 0);
    
    return hasPermission;
}

void chatterinoSendNotification(const char* title, const char* body, const char* identifier, const char* avatarPath, bool playSound) {
    if (!chatterinoHasNotificationPermission()) {
        qCWarning(chatterinoMacOSNotification) << "Cannot send notification: no permission";
        return;
    }
    
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Create notification content
    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = [NSString stringWithUTF8String:title];
    content.body = [NSString stringWithUTF8String:body];
    
    // Add sound based on the setting
    if (playSound) {
        content.sound = [UNNotificationSound defaultSound];
    } else {
        content.sound = nil; // No sound
    }
    
    // Try to add avatar image if available
    if (avatarPath && strlen(avatarPath) > 0) {
        QString avatarQPath = QString::fromUtf8(avatarPath);
        QFileInfo avatarFile(avatarQPath);
        
        if (avatarFile.exists() && avatarFile.isFile()) {
            NSString *avatarNSPath = [NSString stringWithQString:avatarQPath];
            NSURL *avatarURL = [NSURL fileURLWithPath:avatarNSPath];
            
            NSError *error = nil;
            UNNotificationAttachment *attachment = [UNNotificationAttachment 
                attachmentWithIdentifier:@"avatar" 
                URL:avatarURL 
                options:nil 
                error:&error];
            
            if (attachment && !error) {
                content.attachments = @[attachment];
                qCDebug(chatterinoMacOSNotification) << "Added avatar attachment";
            } else if (error) {
                qCWarning(chatterinoMacOSNotification) << "Failed to add avatar attachment:" 
                    << QString::fromUtf8([error.localizedDescription UTF8String]);
            }
        }
    }
    
    // Create notification request
    NSString *requestIdentifier = [NSString stringWithUTF8String:identifier];
    UNNotificationRequest *request = [UNNotificationRequest 
        requestWithIdentifier:requestIdentifier 
        content:content 
        trigger:nil]; // nil trigger means show immediately
    
    // Add notification to notification center
    [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
        if (error) {
            qCWarning(chatterinoMacOSNotification) << "Failed to send notification:" 
                << QString::fromUtf8([error.localizedDescription UTF8String]);
        } else {
            qCDebug(chatterinoMacOSNotification) << "Successfully sent notification for" << identifier;
        }
    }];
}

// C wrapper to call back to the C++ code for handling notification clicks
void performReactionForMacOS(const char* channelName) {
    QString qChannelName = QString::fromUtf8(channelName);
    qCDebug(chatterinoMacOSNotification) << "Performing reaction for channel:" << qChannelName;
    
    // Call the C wrapper function from Toasts.cpp
    extern void handleMacOSNotificationClickC(const char* channelName);
    handleMacOSNotificationClickC(channelName);
}

} // extern "C"