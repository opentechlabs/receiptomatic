-- phpMyAdmin SQL Dump
-- version 2.11.11.3
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: May 06, 2011 at 05:13 PM
-- Server version: 5.0.77
-- PHP Version: 5.1.6

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `receiptomatic`
--

-- --------------------------------------------------------

--
-- Table structure for table `approved`
--

CREATE TABLE IF NOT EXISTS `approved` (
  `id` varchar(64) NOT NULL,
  `who` varchar(255) NOT NULL,
  `timestamp` int(10) unsigned NOT NULL,
  `status` smallint(5) unsigned NOT NULL,
  `reason` varchar(255) NOT NULL,
  KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `field_names`
--

CREATE TABLE IF NOT EXISTS `field_names` (
  `username` varchar(255) NOT NULL,
  `receipt_date` varchar(255) NOT NULL,
  `department` varchar(255) NOT NULL,
  `employee_number` varchar(255) NOT NULL,
  `reason` varchar(255) NOT NULL,
  `po_num` varchar(255) NOT NULL,
  `cost_codes` varchar(255) NOT NULL,
  `account_codes` varchar(255) NOT NULL,
  `supplier_name` varchar(255) NOT NULL,
  `supplier_town` varchar(255) NOT NULL,
  `vat_number` varchar(255) NOT NULL,
  `gross_amount` varchar(255) NOT NULL,
  `net_amount` varchar(255) NOT NULL,
  `vat_amount` varchar(255) NOT NULL,
  `vat_rate` varchar(255) NOT NULL,
  `currency` varchar(255) NOT NULL,
  `payment_method` varchar(255) NOT NULL,
  UNIQUE KEY `username` (`username`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `images`
--

CREATE TABLE IF NOT EXISTS `images` (
  `id` varchar(64) NOT NULL,
  `who` varchar(255) NOT NULL,
  `timestamp` int(10) unsigned NOT NULL,
  `path` varchar(255) NOT NULL,
  `name` varchar(255) NOT NULL,
  `processed` tinyint(4) unsigned NOT NULL default '0',
  `approved` tinyint(3) unsigned NOT NULL default '1',
  KEY `id` (`id`),
  KEY `processed` (`processed`),
  KEY `who` (`who`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `passwd`
--

CREATE TABLE IF NOT EXISTS `passwd` (
  `uid` int(10) unsigned NOT NULL,
  `username` varchar(255) NOT NULL,
  `password` varchar(34) NOT NULL,
  `name` varchar(255) NOT NULL,
  `u_email` varchar(255) NOT NULL,
  `type` smallint(5) unsigned NOT NULL default '0',
  KEY `username` (`username`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `tags`
--

CREATE TABLE IF NOT EXISTS `tags` (
  `id` varchar(64) NOT NULL,
  `username` varchar(255) NOT NULL,
  `timestamp` int(10) unsigned NOT NULL,
  `employee_number` varchar(255) NOT NULL,
  `department` varchar(50) NOT NULL,
  `po_num` varchar(30) NOT NULL,
  `cost_codes` varchar(255) NOT NULL,
  `account_codes` varchar(255) NOT NULL,
  `supplier_town` varchar(255) NOT NULL,
  `supplier_name` varchar(255) NOT NULL,
  `currency` varchar(3) NOT NULL,
  `gross_amount` decimal(12,2) NOT NULL,
  `vat_amount` decimal(12,2) NOT NULL,
  `net_amount` decimal(12,2) NOT NULL,
  `vat_rate` decimal(5,2) NOT NULL,
  `vat_number` varchar(20) NOT NULL,
  `receipt_date` int(10) unsigned NOT NULL,
  `reason` varchar(255) NOT NULL,
  `payment_method` varchar(6) NOT NULL,
  KEY `id` (`id`),
  KEY `timestamp` (`timestamp`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
