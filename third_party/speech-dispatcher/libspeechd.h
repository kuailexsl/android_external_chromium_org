
/*
 * libspeechd.h - Shared library for easy acces to Speech Dispatcher functions (header)
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Brailcom, o.p.s.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * $Id: libspeechd.h,v 1.29 2008-07-30 09:47:00 hanke Exp $
 */

#ifndef _LIBSPEECHD_H
#define _LIBSPEECHD_H

#include <stdio.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
    
/* Arguments for spd_send_data() */
#define SPD_WAIT_REPLY 1              /* Wait for reply */
#define SPD_NO_REPLY 0               /* No reply requested */


/* --------------------- Public data types ------------------------ */

typedef enum{
    SPD_PUNCT_ALL = 0,
    SPD_PUNCT_NONE = 1,
    SPD_PUNCT_SOME = 2
}SPDPunctuation;

typedef enum{
    SPD_CAP_NONE = 0,
    SPD_CAP_SPELL = 1,
    SPD_CAP_ICON = 2
}SPDCapitalLetters;

typedef enum{
    SPD_SPELL_OFF = 0,
    SPD_SPELL_ON = 1
}SPDSpelling;

typedef enum{
    SPD_DATA_TEXT = 0,
    SPD_DATA_SSML = 1
}SPDDataMode;
    
typedef enum{
    SPD_MALE1 = 1,
    SPD_MALE2 = 2,
    SPD_MALE3 = 3,
    SPD_FEMALE1 = 4,
    SPD_FEMALE2 = 5,
    SPD_FEMALE3 = 6,
    SPD_CHILD_MALE = 7,
    SPD_CHILD_FEMALE = 8
}SPDVoiceType;


typedef struct{
  char *name;   /* Name of the voice (id) */
  char *language;  /* 2-letter ISO language code */
  char *variant;   /* a not-well defined string describing dialect etc. */
}SPDVoice;

typedef enum{
    SPD_BEGIN = 1,
    SPD_END = 2,
    SPD_INDEX_MARKS = 4,
    SPD_CANCEL = 8,
    SPD_PAUSE = 16,
    SPD_RESUME = 32
}SPDNotification;

typedef enum{
    SPD_IMPORTANT = 1,
    SPD_MESSAGE = 2,
    SPD_TEXT = 3,
    SPD_NOTIFICATION = 4,
    SPD_PROGRESS = 5
}SPDPriority;

typedef enum{
    SPD_EVENT_BEGIN,
    SPD_EVENT_END,
    SPD_EVENT_CANCEL,
    SPD_EVENT_PAUSE,
    SPD_EVENT_RESUME,
    SPD_EVENT_INDEX_MARK
}SPDNotificationType;

typedef enum{
    SPD_MODE_SINGLE = 0,
    SPD_MODE_THREADED = 1
}SPDConnectionMode;

typedef enum{
    SPD_METHOD_UNIX_SOCKET = 0,
    SPD_METHOD_INET_SOCKET = 1,
}SPDConnectionMethod;

typedef struct{
  SPDConnectionMethod method;
  char *unix_socket_name;
  char *inet_socket_host;
  int  inet_socket_port;
  char *dbus_bus;
}SPDConnectionAddress;

typedef void (*SPDCallback)(size_t msg_id, size_t client_id, SPDNotificationType state);
typedef void (*SPDCallbackIM)(size_t msg_id, size_t client_id, SPDNotificationType state, char *index_mark);

typedef struct{

    /* PUBLIC */
    SPDCallback callback_begin;
    SPDCallback callback_end;
    SPDCallback callback_cancel;
    SPDCallback callback_pause;
    SPDCallback callback_resume;
    SPDCallbackIM callback_im;

    /* PRIVATE */
    int socket;
    FILE *stream;
    SPDConnectionMode mode;

    pthread_mutex_t *ssip_mutex;

    pthread_t *events_thread;
    pthread_mutex_t *comm_mutex;
    pthread_cond_t *cond_reply_ready;
    pthread_mutex_t *mutex_reply_ready;
    pthread_cond_t *cond_reply_ack;
    pthread_mutex_t *mutex_reply_ack;

    char *reply;

}SPDConnection;

/* -------------- Public functions --------------------------*/

/* Openning and closing Speech Dispatcher connection */
SPDConnectionAddress* spd_get_default_address(char** error);
SPDConnection* spd_open(const char* client_name, const char* connection_name, const char* user_name,
			SPDConnectionMode mode);
SPDConnection* spd_open2(const char* client_name, const char* connection_name, const char* user_name,
			 SPDConnectionMode mode, SPDConnectionAddress *address, int autospawn,
			 char **error_result);

void spd_close(SPDConnection* connection);

/* Speaking */
int spd_say(SPDConnection* connection, SPDPriority priority, const char* text);
int spd_sayf(SPDConnection* connection, SPDPriority priority, const char *format, ...);

/* Speech flow */
int spd_stop(SPDConnection* connection);
int spd_stop_all(SPDConnection* connection);
int spd_stop_uid(SPDConnection* connection, int target_uid);

int spd_cancel(SPDConnection* connection);
int spd_cancel_all(SPDConnection* connection);
int spd_cancel_uid(SPDConnection* connection, int target_uid);

int spd_pause(SPDConnection* connection);
int spd_pause_all(SPDConnection* connection);
int spd_pause_uid(SPDConnection* connection, int target_uid);

int spd_resume(SPDConnection* connection);
int spd_resume_all(SPDConnection* connection);
int spd_resume_uid(SPDConnection* connection, int target_uid);

/* Characters and keys */
int spd_key(SPDConnection* connection, SPDPriority priority, const char *key_name);
int spd_char(SPDConnection* connection, SPDPriority priority, const char *character);
int spd_wchar(SPDConnection* connection, SPDPriority priority, wchar_t wcharacter);

/* Sound icons */
int spd_sound_icon(SPDConnection* connection, SPDPriority priority, const char *icon_name);

/* Setting parameters */
int spd_set_voice_type(SPDConnection*, SPDVoiceType type);
int spd_set_voice_type_all(SPDConnection*, SPDVoiceType type);
int spd_set_voice_type_uid(SPDConnection*, SPDVoiceType type, unsigned int uid);

int spd_set_synthesis_voice(SPDConnection*, const char *voice_name);
int spd_set_synthesis_voice_all(SPDConnection*, const char *voice_name);
int spd_set_synthesis_voice_uid(SPDConnection*, const char *voice_name, unsigned int uid);

int spd_set_data_mode(SPDConnection *connection, SPDDataMode mode);

int spd_set_notification_on(SPDConnection* connection, SPDNotification notification);
int spd_set_notification_off(SPDConnection* connection, SPDNotification notification);
int spd_set_notification(SPDConnection* connection, SPDNotification notification, const char* state);

int spd_set_voice_rate(SPDConnection* connection, signed int rate);
int spd_set_voice_rate_all(SPDConnection* connection, signed int rate);
int spd_set_voice_rate_uid(SPDConnection* connection, signed int rate, unsigned int uid);

int spd_set_voice_pitch(SPDConnection* connection, signed int pitch);
int spd_set_voice_pitch_all(SPDConnection* connection, signed int pitch);
int spd_set_voice_pitch_uid(SPDConnection* connection, signed int pitch, unsigned int uid);

int spd_set_volume(SPDConnection* connection, signed int volume);
int spd_set_volume_all(SPDConnection* connection, signed int volume);
int spd_set_volume_uid(SPDConnection* connection, signed int volume, unsigned int uid);

int spd_set_punctuation(SPDConnection* connection, SPDPunctuation type);
int spd_set_punctuation_all(SPDConnection* connection, SPDPunctuation type);
int spd_set_punctuation_uid(SPDConnection* connection, SPDPunctuation type, unsigned int uid);

int spd_set_capital_letters(SPDConnection* connection, SPDCapitalLetters type);
int spd_set_capital_letters_all(SPDConnection* connection, SPDCapitalLetters type);
int spd_set_capital_letters_uid(SPDConnection* connection, SPDCapitalLetters type, unsigned int uid);

int spd_set_spelling(SPDConnection* connection, SPDSpelling type);
int spd_set_spelling_all(SPDConnection* connection, SPDSpelling type);
int spd_set_spelling_uid(SPDConnection* connection, SPDSpelling type, unsigned int uid);

int spd_set_language(SPDConnection* connection, const char* language);
int spd_set_language_all(SPDConnection* connection, const char* language);
int spd_set_language_uid(SPDConnection* connection, const char* language, unsigned int uid);

int spd_set_output_module(SPDConnection* connection, const char* output_module);
int spd_set_output_module_all(SPDConnection* connection, const char* output_module);
int spd_set_output_module_uid(SPDConnection* connection, const char* output_module, unsigned int uid);

int spd_get_client_list(SPDConnection *connection, char **client_names, int *client_ids, int* active);
int spd_get_message_list_fd(SPDConnection *connection, int target, int *msg_ids, char **client_names);

char** spd_list_modules(SPDConnection *connection);
char** spd_list_voices(SPDConnection *connection);
SPDVoice** spd_list_synthesis_voices(SPDConnection *connection);
char** spd_execute_command_with_list_reply(SPDConnection *connection, char* command);


/* Direct SSIP communication */
int spd_execute_command(SPDConnection* connection, char* command);
int spd_execute_command_with_reply(SPDConnection *connection, char* command, char **reply);
int spd_execute_command_wo_mutex(SPDConnection *connection, char* command);
char* spd_send_data(SPDConnection* connection, const char *message, int wfr);
char* spd_send_data_wo_mutex(SPDConnection *connection, const char *message, int wfr);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ifndef _LIBSPEECHD_H */
