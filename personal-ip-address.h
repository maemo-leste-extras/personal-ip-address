/*
 * This file is part of Personal IP Address.
 *
 * Copyright (C) 2009 Andrew Olmsted. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
*  
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/* personal-ip-address.h */

#ifndef _PERSONAL_IP_ADDRESS
#define _PERSONAL_IP_ADDRESS

#include <libhildondesktop/libhildondesktop.h>
#include <time.h> /* For tm. */

G_BEGIN_DECLS

#define PERSONAL_TYPE_IP_ADDRESS personal_ip_address_get_type()

#define PERSONAL_IP_ADDRESS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PERSONAL_TYPE_IP_ADDRESS, PersonalIpAddress))

#define PERSONAL_IP_ADDRESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PERSONAL_TYPE_IP_ADDRESS, PersonalIpAddressClass))

#define PERSONAL_IS_IP_ADDRESS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PERSONAL_TYPE_IP_ADDRESS))

#define PERSONAL_IS_IP_ADDRESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PERSONAL_TYPE_IP_ADDRESS))

#define PERSONAL_IP_ADDRESS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PERSONAL_TYPE_IP_ADDRESS, PersonalIpAddressClass))

typedef struct _PersonalIpAddress        PersonalIpAddress;
typedef struct _PersonalIpAddressClass   PersonalIpAddressClass;
typedef struct _PersonalIpAddressPrivate PersonalIpAddressPrivate;

struct _PersonalIpAddress
{
  HDHomePluginItem parent;

  PersonalIpAddressPrivate *priv;
};

struct _PersonalIpAddressClass
{
  HDHomePluginItemClass  parent;
};

GType personal_ip_address_get_type (void);

G_END_DECLS

#endif /* _PERSONAL_IP_ADDRESS */

