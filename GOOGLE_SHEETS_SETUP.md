# Google Sheets Setup Instructions

Follow these steps to set up Google Sheets to receive data from your ESP32.

---

## Step 1: Create a New Google Sheet

1. Go to [Google Sheets](https://sheets.google.com)
2. Click **+ Blank** to create a new spreadsheet
3. Name it **"MonitoRing Data"** (or any name you prefer)

---

## Step 2: Open Apps Script

1. In your new Google Sheet, click **Extensions** → **Apps Script**
2. A new tab will open with the Apps Script editor
3. Delete any existing code in the editor

---

## Step 3: Paste the Script Code

1. Open the file `google_apps_script.js` from your MonitoRing folder
2. Copy ALL the code
3. Paste it into the Apps Script editor
4. Click **File** → **Save** (or Ctrl+S)
5. Name the project **"MonitoRing"**

---

## Step 4: Set Up Headers

1. In the Apps Script editor, find the function dropdown (top toolbar)
2. Select **setupHeaders** from the dropdown
3. Click **Run** (play button)
4. **First time only:** You'll see a permission popup
   - Click **Review permissions**
   - Select your Google account
   - Click **Advanced** → **Go to MonitoRing (unsafe)**
   - Click **Allow**
5. Go back to your Google Sheet tab - you should see the headers now

---

## Step 5: Deploy as Web App

1. In Apps Script, click **Deploy** → **New deployment**
2. Click the gear icon ⚙️ next to "Select type" and choose **Web app**
3. Fill in:
   - **Description:** "MonitoRing v1"
   - **Execute as:** "Me"
   - **Who has access:** "Anyone"
4. Click **Deploy**
5. **IMPORTANT:** Copy the **Web app URL** that appears
   - It looks like: `https://script.google.com/macros/s/XXXXX.../exec`
6. Click **Done**

---

## Step 6: Update Arduino Code

1. Open `arduinocoding.ino` in Arduino IDE
2. Find this line near the top:
   ```cpp
   const char* googleScriptURL = "YOUR_GOOGLE_SCRIPT_URL";
   ```
3. Replace `YOUR_GOOGLE_SCRIPT_URL` with the URL you copied
4. Also update your WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

---

## Troubleshooting

### Data not appearing in sheet?
- Check Serial Monitor for error messages
- Verify the URL is correct (no extra spaces)
- Make sure WiFi is connected

### Permission errors?
- Redeploy the Web App
- Make sure "Who has access" is set to "Anyone"

### Want to update the script?
- After editing, go to **Deploy** → **Manage deployments**
- Click the edit pencil ✏️ on your deployment
- Select **New version** under "Version"
- Click **Deploy**

---

## Done!

Your Google Sheet will now receive data from ESP32 every 5 seconds!
