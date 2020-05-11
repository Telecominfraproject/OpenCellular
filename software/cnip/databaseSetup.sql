
--
-- 
-- 
--

--DROP DATABASE IF EXISTS cnip;

--CREATE DATABASE cnip;

--
-- 
-- 
--
-- https://postgis.net/install/

-- Enable PostGIS (as of 3.0 contains just geometry/geography)
CREATE EXTENSION postgis;
-- Enable Topology
CREATE EXTENSION postgis_topology;
-- Enable PostGIS Advanced 3D
-- and other geoprocessing algorithms
-- sfcgal not available with all distributions
CREATE EXTENSION postgis_sfcgal;
-- fuzzy matching needed for Tiger
CREATE EXTENSION fuzzystrmatch;
-- rule based standardizer
CREATE EXTENSION address_standardizer;
-- example rule data set
CREATE EXTENSION address_standardizer_data_us;
-- Enable US Tiger Geocoder
CREATE EXTENSION postgis_tiger_geocoder;

--
-- 
-- 
--
DROP TABLE IF EXISTS public.pusers;

CREATE TABLE public.pusers
(
  uid text,
  ukey text,
  name text,
  company text,
  email text,
  pwd text,
  accstatus boolean,
  authsetup boolean,
  authkey text
);

--
-- 
-- 
--
DROP TABLE IF EXISTS public.puserslinks;

CREATE TABLE public.puserslinks
(
  uid character varying,
  linkid character varying,
  linkname character varying,
  linktype character varying,
  siteida character varying,
  locheighta character varying,
  heighta character varying,
  bearinga character varying,
  channelwidtha character varying,
  frequencya character varying,
  outputpowera character varying,
  antennagaina character varying,
  lossesa character varying,
  siteidb character varying,
  locheightb character varying,
  heightb character varying,
  bearingb character varying,
  channelwidthb character varying,
  frequencyb character varying,
  outputpowerb character varying,
  antennagainb character varying,
  lossesb character varying,
  distance character varying,
  elevstr character varying,
  name character varying,
  email character varying,
  wkb_geometry geometry(LineString,4326)
);

--
-- 
-- 
--
DROP TABLE IF EXISTS public.puserslog;

CREATE TABLE public.puserslog
(
  uid text,
  period timestamp without time zone,
  activity text,
  location text
);

--
-- 
-- 
--
DROP TABLE IF EXISTS public.puserslogin;

CREATE TABLE public.puserslogin
(
  uid text,
  attempt double precision
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.pusersnetwork;

CREATE TABLE public.pusersnetwork
(
  uid character varying,
  siteid character varying,
  sitename character varying,
  longitude double precision,
  latitude double precision,
  height character varying,
  region character varying,
  country character varying,
  city character varying,
  district character varying,
  province character varying,
  address character varying,
  comments character varying,
  technology character varying,
  band character varying,
  bandwidth character varying,
  cellid character varying,
  lac character varying,
  rfcn character varying,
  rfid character varying,
  dlfrequency character varying,
  ulfrequency character varying,
  rfpower character varying,
  hba character varying,
  azimuth character varying,
  antennamodel character varying,
  antennatype character varying,
  polarization character varying,
  vbeamwidth character varying,
  hbeamwidth character varying,
  downtilt character varying,
  antennagain character varying,
  feederloss character varying,
  wkb_geometry geometry(Point,4326)
);

--
-- 
-- 
--
DROP TABLE IF EXISTS public.pusersnotes;

CREATE TABLE public.pusersnotes
(
  uid character varying,
  noteid character varying,
  notename character varying,
  notetype character varying,
  description character varying,
  wkb_geometry geometry(Point,4326)
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.puserspolygons;

CREATE TABLE public.puserspolygons
(
  uid character varying,
  polygonid character varying,
  polygonname character varying,
  wkb_geometry geometry(Polygon,4326)
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.pusersresults;

CREATE TABLE public.pusersresults
(
  uid character varying,
  resultid character varying,
  resulttype character varying,
  resultname character varying,
  resultstring character varying,
  wkb_geometry geometry(Point,4326)
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.puserssettings;

CREATE TABLE public.puserssettings
(
  uid text,
  pl_measurementtype text,
  pl_terrainresolution text,
  pl_thematic text,
  pl_radius double precision,
  pl_propagationmodel text,
  pl_reliabilitys double precision,
  pl_reliabilityt double precision,
  pl_terrainconductivity text,
  pl_radioclimate text,
  pl_receiverheight double precision,
  pl_receivergain double precision,
  pl_receiversensitivity double precision,
  rp_antennamodel text,
  rp_cellradius double precision,
  rp_gsmband text,
  rp_gsmbandwidth text,
  rp_lteband text,
  rp_ltebandwidth text,
  mw_frequency double precision,
  mw_channelwidth double precision,
  mw_outputpower double precision,
  mw_antennagain double precision,
  mw_losses double precision,
  mw_fresnelclearance double precision
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.tdr_default;

CREATE TABLE public.tdr_default 
(
    rate integer,
    color character varying(11),
    weight integer,
    ratestring character varying(9)
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.tfs_default;

CREATE TABLE public.tfs_default 
(
    rate integer,
    color character varying(11),
    weight integer
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.tpl_default;

CREATE TABLE public.tpl_default 
(
    rate integer,
    color character varying(11),
    weight integer
);

--
-- 
-- 
--
DROP TABLE IF EXISTS public.trp_default;

CREATE TABLE public.trp_default 
(
    rate integer,
    color character varying(11),
    weight integer
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.tsn_default;

CREATE TABLE public.tsn_default 
(
    rate integer,
    color character varying(11),
    weight integer
);


--
-- 
-- 
--
DROP TABLE IF EXISTS public.tvq_default;

CREATE TABLE public.tvq_default 
(
    rate integer,
    color character varying(11),
    weight integer,
    ratestring character varying(9)
);

--
-- 
-- 
-- 
--

INSERT INTO public.pusers SELECT '1', '', 'Ali Raza Anis', 'CNIP', 'alirazaanis@cnip.com', 'Admin123', 't', 'f', '';
INSERT INTO public.pusers SELECT '2', '', 'Fahad Khalid', 'CNIP', 'fahadkhalid@cnip.com', 'Admin123', 't', 'f', '';
INSERT INTO public.pusers SELECT '3', '', 'Kashif Ali', 'CNIP', 'kashifali@cnip.com', 'Admin123', 't', 'f', '';

--
-- 
-- 
-- 
--


INSERT INTO public.tdr_default (rate, color, weight, ratestring) VALUES (1, '255,0,255', 10, 'Bad');
INSERT INTO public.tdr_default (rate, color, weight, ratestring) VALUES (2, '0,38,255', 20, 'Poor');
INSERT INTO public.tdr_default (rate, color, weight, ratestring) VALUES (3, '0,255,0', 30, 'Fair');
INSERT INTO public.tdr_default (rate, color, weight, ratestring) VALUES (4, '255,255,0', 40, 'Good');
INSERT INTO public.tdr_default (rate, color, weight, ratestring) VALUES (5, '255,0,0', 50, 'Excellent');


--
-- 
-- 
-- 
--

INSERT INTO public.tfs_default (rate, color, weight) VALUES (8, '140,0,128', 10);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (18, '142,63,255', 20);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (28, '0,38,255', 30);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (38, '80,80,255', 40);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (48, '0,148,255', 50);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (58, '0,196,196', 60);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (68, '0,208,0', 70);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (78, '0,255,0', 80);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (88, '184,255,0', 90);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (98, '255,255,0', 100);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (108, '255,206,0', 110);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (118, '255,165,0', 120);
INSERT INTO public.tfs_default (rate, color, weight) VALUES (128, '255,0,0', 130);


--
-- 
-- 
-- 
--

INSERT INTO public.tpl_default (rate, color, weight) VALUES (155, '255,194,204', 10);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (150, '255,0,255', 20);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (145, '196,54,255', 30);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (140, '142,63,255', 40);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (135, '0,38,255', 50);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (130, '80,80,255', 60);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (125, '0,148,255', 70);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (120, '0,196,196', 80);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (115, '0,208,0', 90);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (110, '0,255,0', 100);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (105, '184,255,0', 110);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (100, '255,255,0', 120);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (95, '255,206,0', 130);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (90, '255,165,0', 140);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (85, '255,128,0', 150);
INSERT INTO public.tpl_default (rate, color, weight) VALUES (80, '255,0,0', 160);


--
-- 
-- 
-- 
--

INSERT INTO public.trp_default (rate, color, weight) VALUES (-150, '255,194,204', 0.1);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-140, '255,0,255', 0.2);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-130, '196,54,255', 0.4);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-120, '142,63,255', 0.8);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-110, '0,38,255', 1);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-100, '80,80,255', 2);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-90, '0,148,255', 4);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-80, '0,196,196', 8);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-70, '0,208,0', 16);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-60, '0,255,0', 32);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-50, '184,255,0', 64);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-40, '255,255,0', 128);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-30, '255,206,0', 256);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-20, '255,165,0', 512);
INSERT INTO public.trp_default (rate, color, weight) VALUES (-10, '255,128,0', 1024);
INSERT INTO public.trp_default (rate, color, weight) VALUES (0, '255,0,0', 2048);


--
-- 
-- 
-- 
--

INSERT INTO public.tsn_default (rate, color, weight) VALUES (-5, '196,54,255', 10);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (0, '142,63,255', 20);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (5, '0,38,255', 30);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (10, '80,80,255', 40);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (15, '0,148,255', 50);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (20, '0,196,196', 60);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (25, '0,208,0', 70);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (30, '0,255,0', 80);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (35, '184,255,0', 90);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (40, '255,255,0', 100);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (45, '255,206,0', 110);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (50, '255,165,0', 120);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (55, '255,128,0', 130);
INSERT INTO public.tsn_default (rate, color, weight) VALUES (60, '255,0,0', 140);


--
-- 
-- 
-- 
--

INSERT INTO public.tvq_default (rate, color, weight, ratestring) VALUES (1, '255,0,255', 10, 'Bad');
INSERT INTO public.tvq_default (rate, color, weight, ratestring) VALUES (2, '0,38,255', 20, 'Poor');
INSERT INTO public.tvq_default (rate, color, weight, ratestring) VALUES (3, '0,255,0', 30, 'Fair');
INSERT INTO public.tvq_default (rate, color, weight, ratestring) VALUES (4, '255,255,0', 40, 'Good');
INSERT INTO public.tvq_default (rate, color, weight, ratestring) VALUES (5, '255,0,0', 50, 'Excellent');
