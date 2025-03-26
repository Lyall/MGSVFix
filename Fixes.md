# Ground Zeroes

*Ground Zeroes* shares a lot of similar issues with *The Phantom Pain*, they will be covered in that section. Below are a few fixed issues that are specific to *Ground Zeroes*.

## Borderless Mode  

When **borderless windowed mode** is enabled, the game sets the `HWND_TOPMOST` flag using [`SetWindowPos`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos).  
This forces the window to always stay on top when attempting to **ALT+TAB**, making the borderless mode a bit pointless.  

![flags](https://github.com/user-attachments/assets/35185a31-52dd-4970-aba5-d2c54860edc3)  

**MGSVFix removes this flag**, ensuring borderless mode behaves as expected.

## Framerate Unlock  

MGSVFix includes an **option to unlock the framerate**. This replicates the effect of modifying [`TPP_GRAPHICS_CONFIG`](https://www.pcgamingwiki.com/wiki/Metal_Gear_Solid_V:_The_Phantom_Pain#High_frame_rate) and setting `framerate_control` to `Variable`.  

While this works well in *The Phantom Pain*, it introduces physics issues in *Ground Zeroes*.  
 
At higher framerates (above ~75 FPS), **throwables like magazines or grenades freeze in mid-air**, as shown in this clip:  

https://github.com/user-attachments/assets/d1dcabf2-468f-48c6-a85f-f488b9cef306

MGSVFix **corrects this issue**, allowing throwables to work properly regardless of framerate.  

https://github.com/user-attachments/assets/a6497b1f-1e48-48e7-a846-4bab1c781036

---

# The Phantom Pain

Each issue listed below is address by MGSVFix.

### Unlock Resolutions

Both *The Phantom Pain* and *Ground Zeroes* only support resolutions that are 16:9 or 16:10. MGSVFix removes this restriction and allows the game to display all available resolutions.

![ezgif-7b4ab7b4ab66e7](https://github.com/user-attachments/assets/6bbb8585-0f69-4032-8bf1-6cfd2b7aa20b)

## Ultrawide

### Depth of Field

Depth of field is exaggerated at ultrawide resolutions. This can be a little hard to see in a static comparison, and is much easier to see in motion.

![ezgif-785274e5eb8365](https://github.com/user-attachments/assets/c25eabe5-6352-4cf9-ace9-b06b3c830739)

### Lens Flares/Effects

At ultrawide resolutions lens flares are exaggerated and over-sized.

![ezgif-7dbb5d44a73a02](https://github.com/user-attachments/assets/49ba2854-d9c9-42ba-9696-4a105de05ca1)

This is especially noticable in the opening of *Ground Zeroes*.

![ezgif-7afcd7c6e344e8](https://github.com/user-attachments/assets/f6d72eb7-670f-441d-b546-fb0ea708623c)

### Overlays

Several overlays, such as the one shown when using the sonar, are scaled incorrectly.

![ezgif-7970240ed3768e](https://github.com/user-attachments/assets/e7aa7924-bb41-4ff8-8de0-9c3fa5272369)

### Sonar Markers

Markers that show up after using the sonar are misaligned and incorrectly placed.

![ezgif-714f626a77b91c](https://github.com/user-attachments/assets/4e2bea6c-8a60-4196-8aef-b2844015915c)

### Throwable Marker

When readying a throwable, the destination marker is scaled incorrectly.

![ezgif-7877e6419c7b89](https://github.com/user-attachments/assets/364b5545-8295-4c47-b085-e20d6a1d82bc)

### Markers

Markers are misaligned and incorrectly placed.

![ezgif-782c2c57fbe2af](https://github.com/user-attachments/assets/c81b42d4-4383-46d0-bfef-3363682ce796)

### Scope

When using scopes the frame does not span the screen. MGSVFix partially addresses this issue by stretching the frame to span the screen.

![ezgif-7fae960211cee8](https://github.com/user-attachments/assets/15471535-4943-4f9d-ae14-4e0e8521d25f)

### Backgrounds

Several menu backgrounds do not span to fill the screen.

![ezgif-7332ad5afa87eb](https://github.com/user-attachments/assets/9c9fa39b-7ac6-4575-af93-6bc93985f801)

### Movies

While FMV sections in *The Phantom Pain* are pretty rare, they display incorrectly on an ultrawide display. The video itself is stretched to span the screen. A 16:9 video frame is then drawn over the top.

![ezgif-7503f2076a3d8b](https://github.com/user-attachments/assets/f0ed7293-b971-41a3-ab21-54e8d0d263cf)

