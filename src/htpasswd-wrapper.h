#ifndef _INCLUDE_HTPASSWD_WRAPPER
#define _INCLUDE_HTPASSWD_WRAPPER

int IsValidPassword( char* str );
int HTPasswdValidUser( char* lockPath, char* passwdFile, char* userID );
int HTPasswdAddUser( char* lockPath, char* passwdFile, char* userID, char* password );
int HTPasswdRemoveUser( char* lockPath, char* passwdFile, char* userID );
int HTPasswdResetPassword( char* lockPath, char* passwdFile, char* userID, char* password );
int HTPasswdCheckPassword( char* lockPath, char* passwdFile, char* userID, char* password );
int HTPasswdChangePassword( char* lockPath, char* passwdFile, char* userID, char* oldp, char* newp );

#endif
