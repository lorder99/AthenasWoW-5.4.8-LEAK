/* soapService.cpp
   Generated by gSOAP 2.8.32 for executeCommand.h

gSOAP XML Web services tools
Copyright (C) 2000-2016, Robert van Engelen, Genivia Inc. All Rights Reserved.
The soapcpp2 tool and its generated software are released under the GPL.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
--------------------------------------------------------------------------------
A commercial use license is available from Genivia Inc., contact@genivia.com
--------------------------------------------------------------------------------
*/

#include "soapService.h"

Service::Service()
{	this->soap = soap_new();
	this->soap_own = true;
	Service_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

Service::Service(const Service& rhs)
{	this->soap = rhs.soap;
	this->soap_own = false;
}

Service::Service(struct soap *_soap)
{	this->soap = _soap;
	this->soap_own = false;
	Service_init(_soap->imode, _soap->omode);
}

Service::Service(soap_mode iomode)
{	this->soap = soap_new();
	this->soap_own = true;
	Service_init(iomode, iomode);
}

Service::Service(soap_mode imode, soap_mode omode)
{	this->soap = soap_new();
	this->soap_own = true;
	Service_init(imode, omode);
}

Service::~Service()
{	if (this->soap_own)
		soap_free(this->soap);
}

void Service::Service_init(soap_mode imode, soap_mode omode)
{	soap_imode(this->soap, imode);
	soap_omode(this->soap, omode);
	static const struct Namespace namespaces[] = {
        {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
        {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
        {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
        {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
        {"ns1", "urn:TC", NULL, NULL},
        {NULL, NULL, NULL, NULL}
    };
	soap_set_namespaces(this->soap, namespaces);
}

void Service::destroy()
{	soap_destroy(this->soap);
	soap_end(this->soap);
}

void Service::reset()
{	this->destroy();
	soap_done(this->soap);
	soap_initialize(this->soap);
	Service_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

#ifndef WITH_PURE_VIRTUAL
Service *Service::copy()
{	Service *dup = SOAP_NEW_COPY(Service);
	if (dup)
		soap_copy_context(dup->soap, this->soap);
	return dup;
}
#endif

Service& Service::operator=(const Service& rhs)
{	if (this->soap_own)
		soap_free(this->soap);
	this->soap = rhs.soap;
	this->soap_own = false;
	return *this;
}

int Service::soap_close_socket()
{	return soap_closesock(this->soap);
}

int Service::soap_force_close_socket()
{	return soap_force_closesock(this->soap);
}

int Service::soap_senderfault(const char *string, const char *detailXML)
{	return ::soap_sender_fault(this->soap, string, detailXML);
}

int Service::soap_senderfault(const char *subcodeQName, const char *string, const char *detailXML)
{	return ::soap_sender_fault_subcode(this->soap, subcodeQName, string, detailXML);
}

int Service::soap_receiverfault(const char *string, const char *detailXML)
{	return ::soap_receiver_fault(this->soap, string, detailXML);
}

int Service::soap_receiverfault(const char *subcodeQName, const char *string, const char *detailXML)
{	return ::soap_receiver_fault_subcode(this->soap, subcodeQName, string, detailXML);
}

void Service::soap_print_fault(FILE *fd)
{	::soap_print_fault(this->soap, fd);
}

#ifndef WITH_LEAN
#ifndef WITH_COMPAT
void Service::soap_stream_fault(std::ostream& os)
{	::soap_stream_fault(this->soap, os);
}
#endif

char *Service::soap_sprint_fault(char *buf, size_t len)
{	return ::soap_sprint_fault(this->soap, buf, len);
}
#endif

void Service::soap_noheader()
{	this->soap->header = NULL;
}

::SOAP_ENV__Header *Service::soap_header()
{	return this->soap->header;
}

int Service::run(int port)
{	if (!soap_valid_socket(this->soap->master) && !soap_valid_socket(this->bind(NULL, port, 100)))
		return this->soap->error;
	for (;;)
	{	if (!soap_valid_socket(this->accept()))
		{	if (this->soap->errnum == 0) // timeout?
				this->soap->error = SOAP_OK;
			break;
		}
		if (this->serve())
			break;
		this->destroy();
	}
	return this->soap->error;
}

#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
int Service::ssl_run(int port)
{	if (!soap_valid_socket(this->soap->master) && !soap_valid_socket(this->bind(NULL, port, 100)))
		return this->soap->error;
	for (;;)
	{	if (!soap_valid_socket(this->accept()))
		{	if (this->soap->errnum == 0) // timeout?
				this->soap->error = SOAP_OK;
			break;
		}
		if (this->ssl_accept() || this->serve())
			break;
		this->destroy();
	}
	return this->soap->error;
}
#endif

SOAP_SOCKET Service::bind(const char *host, int port, int backlog)
{	return soap_bind(this->soap, host, port, backlog);
}

SOAP_SOCKET Service::accept()
{	return soap_accept(this->soap);
}

#if defined(WITH_OPENSSL) || defined(WITH_GNUTLS)
int Service::ssl_accept()
{	return soap_ssl_accept(this->soap);
}
#endif

int Service::serve()
{
#ifndef WITH_FASTCGI
	unsigned int k = this->soap->max_keep_alive;
#endif
	do
	{

#ifndef WITH_FASTCGI
		if (this->soap->max_keep_alive > 0 && !--k)
			this->soap->keep_alive = 0;
#endif

		if (soap_begin_serve(this->soap))
		{	if (this->soap->error >= SOAP_STOP)
				continue;
			return this->soap->error;
		}
		if (dispatch() || (this->soap->fserveloop && this->soap->fserveloop(this->soap)))
		{
#ifdef WITH_FASTCGI
			soap_send_fault(this->soap);
#else
			return soap_send_fault(this->soap);
#endif
		}

#ifdef WITH_FASTCGI
		soap_destroy(this->soap);
		soap_end(this->soap);
	} while (1);
#else
	} while (this->soap->keep_alive);
#endif
	return SOAP_OK;
}

static int serve_ns1__executeCommand(struct soap*, Service*);

int Service::dispatch()
{	return dispatch(this->soap);
}

int Service::dispatch(struct soap* soap)
{
	Service_init(soap->imode, soap->omode);

	soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "ns1:executeCommand"))
		return serve_ns1__executeCommand(soap, this);
	return soap->error = SOAP_NO_METHOD;
}

static int serve_ns1__executeCommand(struct soap *soap, Service *service)
{	struct ns1__executeCommand soap_tmp_ns1__executeCommand;
	struct ns1__executeCommandResponse soap_tmp_ns1__executeCommandResponse;
	char * soap_tmp_string;
	soap_default_ns1__executeCommandResponse(soap, &soap_tmp_ns1__executeCommandResponse);
	soap_tmp_string = NULL;
	soap_tmp_ns1__executeCommandResponse.result = &soap_tmp_string;
	soap_default_ns1__executeCommand(soap, &soap_tmp_ns1__executeCommand);
	if (!soap_get_ns1__executeCommand(soap, &soap_tmp_ns1__executeCommand, "ns1:executeCommand", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = service->executeCommand(soap_tmp_ns1__executeCommand.command, soap_tmp_ns1__executeCommandResponse.result);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_ns1__executeCommandResponse(soap, &soap_tmp_ns1__executeCommandResponse);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_ns1__executeCommandResponse(soap, &soap_tmp_ns1__executeCommandResponse, "ns1:executeCommandResponse", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_ns1__executeCommandResponse(soap, &soap_tmp_ns1__executeCommandResponse, "ns1:executeCommandResponse", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}
/* End of server object code */
