import { test, expect } from '@playwright/test';

test.describe('NMMiner Dashboard Visual Validation', () => {
  const baseUrl = 'http://127.0.0.1:8788';

  test('dashboard layout at 1280px desktop width', async ({ page }) => {
    await page.setViewportSize({ width: 1280, height: 800 });
    await page.goto(baseUrl, { waitUntil: 'domcontentloaded' });

    // Wait for elements to be available
    await page.waitForSelector('#sd-panel', { timeout: 5000 }).catch(() => {});
    await page.waitForSelector('#speaker-panel', { timeout: 5000 }).catch(() => {});

    // Check for console errors
    const consoleErrors: string[] = [];
    page.on('console', (msg) => {
      if (msg.type() === 'error') {
        consoleErrors.push(msg.text());
      }
    });

    // Check for horizontal overflow
    const bodyScrollWidth = await page.evaluate(() => document.body.scrollWidth);
    expect(bodyScrollWidth).toBeLessThanOrEqual(1280 + 1);

    // Verify new panels exist
    const sdPanel = await page.$('#sd-panel');
    expect(sdPanel).toBeTruthy();

    const speakerPanel = await page.$('#speaker-panel');
    expect(speakerPanel).toBeTruthy();

    const fileList = await page.$('#file-list');
    expect(fileList).toBeTruthy();

    const volumeSlider = await page.$('#volume-slider');
    expect(volumeSlider).toBeTruthy();

    const muteToggle = await page.$('#mute-toggle');
    expect(muteToggle).toBeTruthy();

    const testToneBtn = await page.$('#test-tone-btn');
    expect(testToneBtn).toBeTruthy();

    // Verify no console errors occurred
    expect(consoleErrors.length).toBe(0);

    // Take screenshot
    await page.screenshot({ path: 'tests/reports/dashboard-1280px.png', fullPage: false });
  });

  test('dashboard layout at 375px mobile width', async ({ page }) => {
    await page.setViewportSize({ width: 375, height: 667 });
    await page.goto(baseUrl, { waitUntil: 'domcontentloaded' });

    // Wait for elements
    await page.waitForSelector('#sd-panel', { timeout: 5000 }).catch(() => {});
    await page.waitForSelector('#speaker-panel', { timeout: 5000 }).catch(() => {});

    // Check for console errors
    const consoleErrors: string[] = [];
    page.on('console', (msg) => {
      if (msg.type() === 'error') {
        consoleErrors.push(msg.text());
      }
    });

    // Check for horizontal overflow
    const bodyScrollWidth = await page.evaluate(() => document.body.scrollWidth);
    expect(bodyScrollWidth).toBeLessThanOrEqual(375 + 1);

    // Verify panels still exist
    const sdPanel = await page.$('#sd-panel');
    expect(sdPanel).toBeTruthy();

    const speakerPanel = await page.$('#speaker-panel');
    expect(speakerPanel).toBeTruthy();

    // Verify volume slider is present
    const volumeSlider = await page.$('#volume-slider');
    expect(volumeSlider).toBeTruthy();

    // Verify no console errors
    expect(consoleErrors.length).toBe(0);

    // Take screenshot
    await page.screenshot({ path: 'tests/reports/dashboard-375px.png', fullPage: false });
  });
});
