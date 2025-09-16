#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request notification permission from the user.
 * This should be called once when the application starts.
 * The permission request is asynchronous.
 */
void chatterinoRequestNotificationPermission();

/**
 * Check if notification permission has been granted.
 * @return true if permission is granted, false otherwise
 */
bool chatterinoHasNotificationPermission();

/**
 * Send a macOS notification using UNUserNotificationCenter.
 * @param title The notification title
 * @param body The notification body text
 * @param identifier A unique identifier for this notification
 * @param avatarPath Path to the avatar image file (can be NULL)
 * @param playSound Whether to play the notification sound
 */
void chatterinoSendNotification(const char* title, const char* body, const char* identifier, const char* avatarPath, bool playSound);

/**
 * Called when a notification is clicked to perform the default action.
 * This function calls back into Chatterino's notification handling logic.
 * @param channelName The channel name extracted from the notification identifier
 */
void performReactionForMacOS(const char* channelName);

/**
 * C wrapper for handling notification clicks.
 * @param channelName The channel name as a C string
 */
void handleMacOSNotificationClickC(const char* channelName);

#ifdef __cplusplus
}
#endif