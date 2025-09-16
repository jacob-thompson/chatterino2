#include "MacOSNotifications.h"

#ifdef Q_OS_MACOS

#import <UserNotifications/UserNotifications.h>
#import <Foundation/Foundation.h>

#include <QString>
#include <QDebug>
#include <QFileInfo>
#include "common/QLogging.hpp"

Q_LOGGING_CATEGORY(chatterinoMacOSNotification, "chatterino.macos.notification");

// Helper function to convert QString to NSString
@interface NSString (QString)
+ (NSString *)stringWithQString:(const QString &)qstring;
@end

@implementation NSString (QString)
+ (NSString *)stringWithQString:(const QString &)qstring {
    return [NSString stringWithUTF8String:qstring.toUtf8().constData()];
}
@end

// Global permission status to avoid repeated authorization requests
static BOOL g_permissionRequested = NO;
static BOOL g_permissionGranted = NO;

// C wrapper function implementations
extern "C" {

void chatterinoRequestNotificationPermission() {
    if (g_permissionRequested) {
        return;
    }
    
    g_permissionRequested = YES;
    
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    UNAuthorizationOptions options = UNAuthorizationOptionAlert | UNAuthorizationOptionSound;
    
    [center requestAuthorizationWithOptions:options completionHandler:^(BOOL granted, NSError * _Nullable error) {
        g_permissionGranted = granted;
        
        if (error) {
            qCWarning(chatterinoMacOSNotification) << "Notification permission error:" 
                << QString::fromUtf8([error.localizedDescription UTF8String]);
        } else if (granted) {
            qCDebug(chatterinoMacOSNotification) << "Notification permission granted";
        } else {
            qCDebug(chatterinoMacOSNotification) << "Notification permission denied";
        }
    }];
}

bool chatterinoHasNotificationPermission() {
    if (!g_permissionRequested) {
        return false;
    }
    
    // For a more accurate check, we could use getNotificationSettingsWithCompletionHandler
    // but this is simpler and sufficient for our use case
    return g_permissionGranted;
}

void chatterinoSendNotification(const char* title, const char* body, const char* identifier, const char* avatarPath) {
    if (!chatterinoHasNotificationPermission()) {
        qCWarning(chatterinoMacOSNotification) << "Cannot send notification: no permission";
        return;
    }
    
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Create notification content
    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = [NSString stringWithUTF8String:title];
    content.body = [NSString stringWithUTF8String:body];
    
    // Add sound if enabled (we'll check the setting in C++)
    content.sound = [UNNotificationSound defaultSound];
    
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

} // extern "C"

#endif // Q_OS_MACOS