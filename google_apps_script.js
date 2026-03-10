// MonitoRing - Google Apps Script
// This script receives data from ESP32 and logs it to your Google Sheet

function doGet(e) {
    return handleRequest(e);
}

function doPost(e) {
    return handleRequest(e);
}

function handleRequest(e) {
    try {
        // Get the active spreadsheet
        var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

        // Get parameters from ESP32
        var temperature = e.parameter.temperature || "N/A";
        var humidity = e.parameter.humidity || "N/A";
        var gasPPM = e.parameter.gasPPM || "N/A";
        var gasCondition = e.parameter.gasCondition || "N/A";
        var motion = e.parameter.motion || "N/A";

        // Get current timestamp
        var timestamp = new Date();
        var formattedTime = Utilities.formatDate(timestamp, Session.getScriptTimeZone(), "yyyy-MM-dd HH:mm:ss");

        // Gas condition labels
        var conditionLabels = ["", "Excellent", "Good", "Moderate", "Poor", "Hazardous"];
        var gasConditionText = conditionLabels[parseInt(gasCondition)] || gasCondition;

        // Append data to sheet
        sheet.appendRow([
            formattedTime,
            temperature,
            humidity,
            gasPPM,
            gasConditionText,
            motion
        ]);

        // Return success response
        return ContentService.createTextOutput("Success").setMimeType(ContentService.MimeType.TEXT);

    } catch (error) {
        // Return error response
        return ContentService.createTextOutput("Error: " + error.message).setMimeType(ContentService.MimeType.TEXT);
    }
}

// Run this function ONCE to set up the headers
function setupHeaders() {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

    // Clear existing content (optional)
    // sheet.clear();

    // Add headers
    sheet.getRange(1, 1, 1, 6).setValues([[
        "Timestamp",
        "Temperature (°C)",
        "Humidity (%)",
        "Gas (PPM)",
        "Gas Condition",
        "Motion"
    ]]);

    // Format headers
    var headerRange = sheet.getRange(1, 1, 1, 6);
    headerRange.setFontWeight("bold");
    headerRange.setBackground("#4285f4");
    headerRange.setFontColor("white");

    // Auto-resize columns
    for (var i = 1; i <= 6; i++) {
        sheet.autoResizeColumn(i);
    }

    Logger.log("Headers set up successfully!");
}
