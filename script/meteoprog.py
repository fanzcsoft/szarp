#!/usr/bin/python
# -*- coding: iso-8859-2 -*-

# Script fetches and parses weather forecast data from meteoprog.ua website
# in format suitable for szbwriter input.
# There should be parameters configured in SZARP, 3*2 = 6 parameters for 3-days forecast.
#
# Config file /etc/szarp/meteoprog.cfg is required:
#
# [Main]
# url=<direct url to forecast XML data>
# user=<user name to access data>
# password=<password>

from urllib import FancyURLopener
from lxml import etree
from ConfigParser import SafeConfigParser
import datetime

# Config file path
CONFIG = "/etc/szarp/meteoprog.cfg"

# Patterns for creating parameters names
strs = [
		'"Sie�:Prognoza temperatury:temperatura %s 1 dzie�"',
		'"Sie�:Prognoza temperatury:temperatura %s 2 dni"',
		'"Sie�:Prognoza temperatury:temperatura %s 3 dni"'
		]
minmax = [ "minimalna", "maksymalna" ]

# Starting hours of forcast periods
hours = dict()
hours["night"] = 0
hours["morning"] = 5
hours["day"] = 11
hours["evening"] = 17

shift = dict()
shift["night"] = datetime.timedelta(hours=1)
shift["morning"] = datetime.timedelta()
shift["day"] = datetime.timedelta()
shift["evening"] = datetime.timedelta()
shift["last"] = datetime.timedelta(hours=1)

# parsing config file
config = SafeConfigParser()
config.read(CONFIG)
url = config.get('Main', 'url')
user = config.get('Main', 'user')
passwd = config.get('Main', 'password')

# class for passing user/password
class AuthURLopener(FancyURLopener):
	def prompt_user_passwd(self, host, realm):
		return (user, passwd)

# fetch XML data
opener = AuthURLopener()
meteo = opener.open(url)
xml = etree.XML(meteo.read().lstrip(' \t\n'))

# save output
cache_min = []
cache_max = []
i = 0
for date in xml:
	(year, month, day) =  date.get('date').split('-')
	for time in date:
		tmin = time.find('tmin')
		tmax = time.find('tmax')
		cache_min.append((int(tmin.text), datetime.datetime(int(year), int(month), int(day), hours[time.get('name')], 0) - shift[time.get('name')]))
		cache_max.append((int(tmax.text), datetime.datetime(int(year), int(month), int(day), hours[time.get('name')], 0) - shift[time.get('name')]))
	# add data for end of period
	cache_min.append((int(tmin.text), datetime.datetime(int(year), int(month), int(day)) +
		datetime.timedelta(days=1) - shift['last']))
	cache_max.append((int(tmax.text), datetime.datetime(int(year), int(month), int(day)) +
		datetime.timedelta(days=1) - shift['last']))
	i += 1

interval = datetime.timedelta(minutes=10)

t = minmax[0]
for cache in (cache_min, cache_max):
	start = cache[0]
	for end in cache[1:]:
		curr = start[1]
		while curr < end[1]:
			print (strs[(start[1] - cache[0][1]).days] % t), 
			print "%04d %02d %02d %02d %02d" % (curr.year, curr.month, curr.day, curr.hour, curr.minute),
			print start[0]
			curr += interval
		start = end
	t = minmax[1]

