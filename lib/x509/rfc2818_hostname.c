/*
 * Copyright (C) 2002 Andrew McDonald
 * Portions Copyright 2003 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public   
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <gnutls_int.h>
#include <gnutls_ui.h>
#include <compat.h>
#include <x509.h>
#include <dn.h>
#include <common.h>
#include <gnutls/compat8.h>

static int hostname_compare(const char *certname, const char *hostname);

/**
  * gnutls_x509_check_certificates_hostname - This function compares the given hostname with the hostname in the certificate
  * @cert: should contain a DER encoded certificate
  * @hostname: A null terminated string that contains a DNS name
  *
  * This function will check if the given certificate's subject matches
  * the given hostname. This is a basic implementation of the matching 
  * described in RFC2818 (HTTPS), which takes into account wildcards.
  *
  * Returns non zero on success, and zero on failure.
  *
  **/
int gnutls_x509_check_certificates_hostname(const gnutls_datum * cert,
                                const char *hostname)
{
#define MAX_CN 256
   char dnsname[MAX_CN];
   int dnsnamesize;
   int found_dnsname = 0;
   int ret = 0;
   gnutls_x509_dn dn;
   int i = 0;

   /* try matching against:
    *  1) a DNS name as an alternative name (subjectAltName) extension
    *     in the certificate
    *  2) the common name (CN) in the certificate
    *
    *  either of these may be of the form: *.domain.tld
    *
    *  only try (2) if there is no subjectAltName extension of
    *  type dNSName
    */

   /* Check through all included subjectAltName extensions, comparing
    * against all those of type dNSName.
    */
   for (i = 0; !(ret < 0); i++) {

      dnsnamesize = MAX_CN;
      ret =
          gnutls_x509_extract_certificate_subject_alt_name(cert, i,
                                                           dnsname,
                                                           &dnsnamesize);

      if (ret == GNUTLS_SAN_DNSNAME) {
         found_dnsname = 1;
         if (hostname_compare(dnsname, hostname)) {
            return 1;
         }
      }

   }

   if (!found_dnsname) {
      /* not got the necessary extension, use CN instead 
       */
      if (gnutls_x509_extract_certificate_dn(cert, &dn) != 0) {
         /* got an error, can't find a name 
          */
         return 0;
      }

      if (hostname_compare(dn.common_name, hostname)) {
         return 1;
      }
   }

   /* not found a matching name
    */
   return 0;
}

/* compare hostname against certificate, taking account of wildcards
 * return 1 on success or 0 on error 
 */
static int hostname_compare(const char *certname, const char *hostname)
{
   const char *cmpstr1, *cmpstr2;

   if (strlen(certname) == 0 || strlen(hostname) == 0)
      return 0;

   if (strlen(certname) > 2 && strncmp(certname, "*.", 2) == 0) {
      /* a wildcard certificate */

      cmpstr1 = certname + 1;

      /* find the first dot in hostname, compare from there on */
      cmpstr2 = strchr(hostname, '.');

      if (cmpstr2 == NULL) {
         /* error, the hostname we're connecting to is only a local part */
         return 0;
      }

      if (strcmp(cmpstr1, cmpstr2) == 0) {
         return 1;
      }

      return 0;
   }

   if (strcmp(certname, hostname) == 0) {
      return 1;
   }

   return 0;
}

/**
  * gnutls_x509_certificate_check_hostname - This function compares the given hostname with the hostname in the certificate
  * @cert: should contain an gnutls_x509_certificate structure
  * @hostname: A null terminated string that contains a DNS name
  *
  * This function will check if the given certificate's subject matches
  * the given hostname. This is a basic implementation of the matching 
  * described in RFC2818 (HTTPS), which takes into account wildcards.
  *
  * Returns non zero on success, and zero on failure.
  *
  **/
int gnutls_x509_certificate_check_hostname(gnutls_x509_certificate cert,
                                const char *hostname)
{

   char dnsname[MAX_CN];
   int dnsnamesize;
   int found_dnsname = 0;
   int ret = 0;
   int i = 0;

   /* try matching against:
    *  1) a DNS name as an alternative name (subjectAltName) extension
    *     in the certificate
    *  2) the common name (CN) in the certificate
    *
    *  either of these may be of the form: *.domain.tld
    *
    *  only try (2) if there is no subjectAltName extension of
    *  type dNSName
    */

   /* Check through all included subjectAltName extensions, comparing
    * against all those of type dNSName.
    */
   for (i = 0; !(ret < 0); i++) {

      dnsnamesize = sizeof(dnsname);
      ret =
          gnutls_x509_certificate_get_subject_alt_name(cert, i,
                                        dnsname, &dnsnamesize, NULL);

      if (ret == GNUTLS_SAN_DNSNAME) {
         found_dnsname = 1;
         if (hostname_compare(dnsname, hostname)) {
            return 1;
         }
      }

   }

   if (!found_dnsname) {
      /* not got the necessary extension, use CN instead 
       */
      dnsnamesize = sizeof(dnsname);
      if (gnutls_x509_certificate_get_dn_by_oid(cert, OID_X520_COMMON_NAME, 0,
      		dnsname, &dnsnamesize) != 0) {
         /* got an error, can't find a name 
          */
         return 0;
      }

      if (hostname_compare(dnsname, hostname)) {
         return 1;
      }
   }

   /* not found a matching name
    */
   return 0;
}
