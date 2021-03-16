/** --------------------------------------------------------------------------
 *  WebSocketServer.cpp
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  -------------------------------------------------------------------------- 
 **/

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <fcntl.h>
#include "libwebsockets.h"
#include "WebSocketServer.h"

using namespace std;

#ifdef _WIN32
#define LOCAL_RESOURCE_PATH "F:\\cplusplus\\cppWebSockets-master\\examples\\echoServer"
#else
#define LOCAL_RESOURCE_PATH "/usr/share/websocket"
#endif
char *resource_path = LOCAL_RESOURCE_PATH;

const char * get_mimetype(const char *file)
{
	int n = (int)strlen(file);

	if (n < 5)
		return NULL;

	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";

	if (!strcmp(&file[n - 4], ".css"))
		return "text/css";

	if (!strcmp(&file[n - 3], ".js"))
		return "text/javascript";

	return NULL;
}

static int callback_main(   struct lws *wsi, 
                            enum lws_callback_reasons reason, 
                            void *user, 
                            void *in, 
                            size_t len )
{
	unsigned char buf[LWS_PRE + MAX_WEBSOCKET_PAYLOAD];
	int fd = lws_get_socket_fd(wsi);
	WebSocketServer *webServer = (WebSocketServer *)lws_context_user(lws_get_context(wsi));
    
    switch( reason ) {
        case LWS_CALLBACK_ESTABLISHED:
			webServer->onConnectWrapper(fd);
            break;
		case LWS_CALLBACK_HTTP:
		{
			unsigned char buffer[LWS_PRE + MAX_WEBSOCKET_PAYLOAD];
			const char *mimetype;
			char *url = (char *)in;
			char file[256];
			int n;

			strcpy(file, resource_path);
			if (strcmp(url, "/")) {
				if (*(url) != '/')
					strcat(file, "/");
				strncat(file, url, sizeof(file) - strlen(file) - 1);
			}
			else /* default file to serve */
				strcat(file, "/index.html");

			file[sizeof(file) - 1] = '\0';
			mimetype = get_mimetype(file);
			if (!mimetype) {
				lwsl_err("Unknown mimetype for %s\n", file);
				lws_return_http_status(wsi,
					HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, "Unknown Mimetype");
				return -1;
			}

			n = lws_serve_http_file(wsi, file, mimetype, NULL, 0);
			if (n < 0)
				return -1; /* error*/

			break;
		}
        case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			uint8_t *writer = &buf[LWS_PRE];
			while (!webServer->connections[fd]->buffer.empty()) {
				string message = webServer->connections[fd]->buffer.front();
				int msgLen = message.length();
				strcpy((char *)writer, message.c_str());

				int charsSent = lws_write(wsi, writer, msgLen, LWS_WRITE_TEXT);
				if (charsSent < msgLen)
					webServer->onErrorWrapper(fd, string("Error writing to socket"));
				else
					webServer->connections[fd]->buffer.pop_front();
				//lws_rx_flow_control(wsi, 1);
			}
			break;
		}
        case LWS_CALLBACK_RECEIVE:
		{
			webServer->onMessage(fd, (uint8_t *)in, len);
			//lws_rx_flow_control(wsi, 0);
			lws_callback_on_writable(wsi);
			break;
		}
		case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        case LWS_CALLBACK_CLOSED:
			webServer->onDisconnectWrapper( fd );
            break;

        default:
            break;
    }
    return 0;
}


static struct lws_protocols protocols[] = {
	{
		"/",
		callback_main,
		0, // user data struct not used
		MAX_WEBSOCKET_PAYLOAD
	},{ NULL, NULL, 0, 0 } // terminator
};

WebSocketServer::WebSocketServer( int port, const string certPath, const string& keyPath )
{
    this->_port     = port;
    this->_certPath = certPath;
    this->_keyPath  = keyPath; 

    lws_set_log_level( 0, lwsl_emit_syslog ); // We'll do our own logging, thank you.
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof info );
	info.user = this;
    info.port = this->_port;
    info.iface = NULL;
    info.protocols = protocols;
    
    if( !this->_certPath.empty( ) && !this->_keyPath.empty( ) )
    {
       // Util::log( "Using SSL certPath=" + this->_certPath + ". keyPath=" + this->_keyPath + "." );
        info.ssl_cert_filepath        = this->_certPath.c_str( );
        info.ssl_private_key_filepath = this->_keyPath.c_str( );
    } 
    else 
    {
        //Util::log( "Not using SSL" );
        info.ssl_cert_filepath        = NULL;
        info.ssl_private_key_filepath = NULL;
    }
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    // keep alive
    info.ka_time = 60; // 60 seconds until connection is suspicious
    info.ka_probes = 10; // 10 probes after ^ time
    info.ka_interval = 10; // 10s interval for sending probes
    this->_context = lws_create_context( &info );
    if( !this->_context )
        throw "libwebsocket init failed";
    //Util::log( "Server started on port " + Util::toString( this->_port ) ); 

    // Some of the libwebsocket stuff is define statically outside the class. This 
    // allows us to call instance variables from the outside.  Unfortunately this
    // means some attributes must be public that otherwise would be private. 
}

WebSocketServer::~WebSocketServer( )
{
    // Free up some memory
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
    {
        Connection* c = it->second;
        this->connections.erase( it->first );
        delete c;
    }
}

void WebSocketServer::onConnectWrapper( int socketID )
{
    Connection* c = new Connection;
    c->createTime = time( 0 );
    this->connections[ socketID ] = c;
    this->onConnect( socketID );
}

void WebSocketServer::onDisconnectWrapper( int socketID )
{
    this->onDisconnect( socketID );
    this->_removeConnection( socketID );
}

void WebSocketServer::onErrorWrapper( int socketID, const string& message )
{
   // Util::log( "Error: " + message + " on socketID '" + Util::toString( socketID ) + "'" ); 
    this->onError( socketID, message );
    this->_removeConnection( socketID );
}

void WebSocketServer::send( int socketID, string data )
{
    // Push this onto the buffer. It will be written out when the socket is writable.
    this->connections[socketID]->buffer.push_back(data);
}

void WebSocketServer::broadcast( string data )
{
	for (map<int, Connection*>::const_iterator it = this->connections.begin(); it != this->connections.end(); ++it)
	{
		this->send(it->first, data);
		lws_callback_on_writable_all_protocol(_context, &protocols[0]);
	}
}

void WebSocketServer::setValue( int socketID, const string& name, const string& value )
{
    this->connections[socketID]->keyValueMap[name] = value;
}

string WebSocketServer::getValue( int socketID, const string& name )
{
    return this->connections[socketID]->keyValueMap[name];
}
int WebSocketServer::getNumberOfConnections( )
{
    return this->connections.size( );
}

void WebSocketServer::run( uint64_t timeout )
{
    while( 1 )
    {
        this->wait( timeout );
    }
}

void WebSocketServer::wait( uint64_t timeout )
{
    if( lws_service( this->_context, timeout ) < 0 )
        throw "Error polling for socket activity.";
}

void WebSocketServer::_removeConnection( int socketID )
{
    Connection* c = this->connections[ socketID ];
    this->connections.erase( socketID );
    delete c;
}
