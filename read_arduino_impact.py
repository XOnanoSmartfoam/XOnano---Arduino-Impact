# -*- coding: utf-8 -*-
"""
Created on Wed May  1 11:30:38 2019

@author: taylor
"""

import csv
import matplotlib.pyplot as plt
from pathlib import Path
import os
import tkinter as tk
import numpy as np
from accessimpactfiles import OpenCsvFile, LoadImpactTesterFolder, OpenExcelFile 
from impactbutt import yesno_window, loopsingle_window, plot_window
from DetectPeaks import detect_peaks

program_directory = Path(os.path.dirname(os.path.abspath(__file__)))
os.chdir(program_directory)

maxRatio = 1023
peakVoltage = 0;

#%% FUNCTIONS

#takes an array of voltage data {data} and adjusts it to match the {volt} level scale
def adjust_voltage(v_data):
    i = 0
    while i < len(v_data):
        v_data[i] = (v_data[i] / maxRatio) * peakVoltage
        i += 1
    return v_data

#adjusts one voltage reading
def adjust_one_voltage(reading):
    return (reading / maxRatio) * peakVoltage
#%%show dialog to pick one or many csv files
parent_directory = LoadImpactTesterFolder(program_directory)
window_text = "Analysis Mode"
prompt_text = "Loop through all Data or pick the ones to analyze?"
analysis_mode = loopsingle_window(tk.Tk(),
                                        window_text,
                                        prompt_text).status

if analysis_mode == 'Select':
    csvList = OpenCsvFile(parent_directory)
else:
    csvList = [str(Path(f"{parent_directory}/{f}")) for f in os.listdir(Path(f"{parent_directory}")) if f.endswith(".csv")]
    
for num, csv_file in enumerate(csvList):
    
    impact_data = [];
    smoothed_data = [];
    millis = [];
    baseTime = 0.0               
    with open(csv_file) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        line_count = 0
        for row in csv_reader:
            if line_count == 0:
                peakVoltage = float(row[0])
            else:
                millis.append(float(row[0]))
                impact_data.append(float(row[1]))
                smoothed_data.append(float(row[2]))
            line_count += 1
             
    impact_data = adjust_voltage(impact_data)
    smoothed_data = adjust_voltage(smoothed_data)
    
    minTime = min(millis)
    maxTime = max(millis)
    interval = (maxTime - minTime) / len(millis)
    
    time = [len(millis)];
    time[0] = 0
    index = 1;
    while(index < len(millis)):
        time.append(interval * index)
        index += 1
      
    averageVoltage = np.mean(impact_data[0:50])
    adjusted_raw = [x - averageVoltage for x in impact_data]
    adjusted_smoothed = [x - averageVoltage for x in smoothed_data]
    
    #%%Analyze the data...!
    #peak voltage, total voltage displacement

    total_voltage_displacement = 0;
    for volt in adjusted_raw:
        total_voltage_displacement += abs(volt)
    total_voltage_displacement = round(total_voltage_displacement, 2)
    peaks = detect_peaks(smoothed_data)
    valleys = detect_peaks(smoothed_data, valley='true')
    
    
    max_peak = 0
    min_peak = 5
    max_peak_index = 0
    min_peak_index = 0
    for peak in peaks:
        if smoothed_data[peak] > max_peak:
            max_peak = smoothed_data[peak]
            max_peak_index = peak
    
    for valley in valleys:
        if smoothed_data[valley] < min_peak:
            min_peak = smoothed_data[valley]
            min_peak_index = valley
            
    MAX_PEAK = str(f"{max_peak:.2f} ")
    MIN_PEAK = str(f"{min_peak:.2f} ")
    #%% Plot everything    
    
    fig = plt.figure(num, figsize=[12,6])
    fig.suptitle("Arduino Impact Analysis")
    
    plt.plot(time, impact_data)
    plt.plot(time, smoothed_data)
    plt.ylim(0,5)
    
    plt.hlines(averageVoltage,0, time[len(time) - 1], color='y', linewidth=0.75)
    plt.plot(time[max_peak_index], smoothed_data[max_peak_index], 'kD')
    plt.text(time[max_peak_index], smoothed_data[max_peak_index] + 0.2,"Max Voltage: " + MAX_PEAK)
    plt.plot(time[min_peak_index], smoothed_data[min_peak_index], 'kD')
    plt.text(time[min_peak_index], smoothed_data[min_peak_index] - 0.2,"Min Voltage: " + MIN_PEAK)
    
    plt.show() 
   
    TVD = str(total_voltage_displacement)
    plt.text(0,4.6,"Total voltage displaced: " + TVD, fontsize = 14)
    