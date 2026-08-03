package api.com.discord;

import annotation.Draft;
import annotation.Intention;
import exception.APIException;
import net.HTTPClient;
import net.JSON;
import nio.ForeignMemory;
import primitive.string;

/**
 * High-performance, zero-allocation Discord Webhook API.
 * Formulates off-heap JSON payloads (text-only, embeds, text & binary file attachments)
 * and transmits them to Discord servers using the FFM libcurl HTTPClient.
 */
// [comment]
// High-performance zero-GC Discord Webhook
// transmitter supporting text, embeds, and binary file uploads
@Draft
@Intention("[comment]")
public final class DiscordWebhook
{
    private static final String MULTIPART_BOUNDARY = "----AntiEngineBoundary7MA4";

    private DiscordWebhook() {}

    // [String implementation]
    // the thing is that the String is a single allocation
    // as a test/placeholder in order to make the apis work
    // by communicating it, albeit its disgusting
    // (personally lol) but its okay

    /**
     * Sends a simple text-only message to a Discord Webhook.
     */
    @Intention("[String implementation] line 27")
    public static void sendText(String webhookUrl, String content, String username, String avatarUrl)
    {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        long obj = JSON.createObject();
        obj = JSON.put(obj, "content", content);
        if (username != null) obj = JSON.put(obj, "username", username);
        if (avatarUrl != null) obj = JSON.put(obj, "avatar_url", avatarUrl);

        long payloadPtr = JSON.build(obj);
        transmit(webhookUrl, payloadPtr, 0L, 0L);
    }

    /**
     * Sends a rich embed message to a Discord Webhook.
     */
    @Intention("[String implementation] line 27")
    public static void sendEmbed(
        String webhookUrl, String content, String username, String avatarUrl,
        String embedTitle, String embedDesc, int color, String imageUrl
    ) {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        // 1. Build Embed Object
        long embed = JSON.createObject();
        if (embedTitle != null) embed = JSON.put(embed, "title", embedTitle);
        if (embedDesc != null) embed = JSON.put(embed, "description", embedDesc);
        if (color != 0) embed = JSON.putInt(embed, "color", color);
        if (imageUrl != null)
        {
            long imgObj = JSON.createObject();
            imgObj = JSON.put(imgObj, "url", imageUrl);
            embed = JSON.putObject(embed, "image", imgObj);
        }

        // 2. Wrap embed inside an array
        long finalizedEmbed = JSON.build(embed);
        long embedsArray = JSON.createArray();
        embedsArray = JSON.addArrayElement(embedsArray, finalizedEmbed);
        string.free(finalizedEmbed);

        // 3. Build main payload
        long payload = JSON.createObject();
        if (content != null) payload = JSON.put(payload, "content", content);
        if (username != null) payload = JSON.put(payload, "username", username);
        if (avatarUrl != null) payload = JSON.put(payload, "avatar_url", avatarUrl);
        payload = JSON.putArray(payload, "embeds", embedsArray);

        long payloadPtr = JSON.build(payload);
        transmit(webhookUrl, payloadPtr, 0L, 0L);
    }

    /**
     * Sends a highly customized premium embed card to a Discord Webhook.
     */
    @Intention("[String implementation] line 27")
    public static void sendRichEmbed(
        String webhookUrl, String content, String username, String avatarUrl,
        String title, String description, int color, 
        String imageUrl, String thumbnailUrl,
        String authorName, String authorUrl, String authorIconUrl,
        String footerText, String footerIconUrl,
        long fieldsArrayPtr
    ) {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        // 1. Build Embed Object
        long embed = JSON.createObject();
        if (title != null) embed = JSON.put(embed, "title", title);
        if (description != null) embed = JSON.put(embed, "description", description);
        if (color != 0) embed = JSON.putInt(embed, "color", color);

        // Thumbnail
        if (thumbnailUrl != null)
        {
            long thumbObj = JSON.createObject();
            thumbObj = JSON.put(thumbObj, "url", thumbnailUrl);
            embed = JSON.putObject(embed, "thumbnail", thumbObj);
        }

        // Image
        if (imageUrl != null)
        {
            long imgObj = JSON.createObject();
            imgObj = JSON.put(imgObj, "url", imageUrl);
            embed = JSON.putObject(embed, "image", imgObj);
        }

        // Author
        if (authorName != null)
        {
            long authorObj = JSON.createObject();
            authorObj = JSON.put(authorObj, "name", authorName);
            if (authorUrl != null) authorObj = JSON.put(authorObj, "url", authorUrl);
            if (authorIconUrl != null) authorObj = JSON.put(authorObj, "icon_url", authorIconUrl);
            embed = JSON.putObject(embed, "author", authorObj);
        }

        // Footer
        if (footerText != null)
        {
            long footerObj = JSON.createObject();
            footerObj = JSON.put(footerObj, "text", footerText);
            if (footerIconUrl != null) footerObj = JSON.put(footerObj, "icon_url", footerIconUrl);
            embed = JSON.putObject(embed, "footer", footerObj);
        }

        // Fields
        if (fieldsArrayPtr != 0L)
        {
            embed = JSON.putArray(embed, "fields", fieldsArrayPtr);
        }

        // 2. Wrap embed inside an array
        long finalizedEmbed = JSON.build(embed);
        long embedsArray = JSON.createArray();
        embedsArray = JSON.addArrayElement(embedsArray, finalizedEmbed);
        string.free(finalizedEmbed);

        // 3. Build main payload
        long payload = JSON.createObject();
        if (content != null) payload = JSON.put(payload, "content", content);
        if (username != null) payload = JSON.put(payload, "username", username);
        if (avatarUrl != null) payload = JSON.put(payload, "avatar_url", avatarUrl);
        payload = JSON.putArray(payload, "embeds", embedsArray);

        long payloadPtr = JSON.build(payload);
        transmit(webhookUrl, payloadPtr, 0L, 0L);
    }

    /**
     * Sends a highly customized premium embed card with a binary file attachment to a Discord Webhook.
     * The file is attached and can be referenced in the embed using "attachment://filename.ext".
     */
    @Intention("[String implementation] line 27")
    public static void sendEmbedWithFile(
        String webhookUrl, String content, String username, String avatarUrl,
        String title, String description, int color, 
        String imageUrl, String thumbnailUrl,
        String authorName, String authorUrl, String authorIconUrl,
        String footerText, String footerIconUrl,
        long fieldsArrayPtr,
        String attachmentName, String attachmentPath
    ) {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        // 1. Read file bytes
        java.io.File file = new java.io.File(attachmentPath);
        if (!file.exists())
        {
            throw new APIException("Attachment file does not exist: " + attachmentPath);
        }
        byte[] fileBytes;
        try
        {
            fileBytes = java.nio.file.Files.readAllBytes(file.toPath());
        }
        catch (Exception e)
        {
            throw new APIException("Failed to read attachment file: " + attachmentPath, e);
        }

        var contentType = getString(attachmentName);

        // 2. Build Embed Object
        long embed = JSON.createObject();
        if (title != null) embed = JSON.put(embed, "title", title);
        if (description != null) embed = JSON.put(embed, "description", description);
        if (color != 0) embed = JSON.putInt(embed, "color", color);

        // Thumbnail
        if (thumbnailUrl != null)
        {
            long thumbObj = JSON.createObject();
            thumbObj = JSON.put(thumbObj, "url", thumbnailUrl);
            embed = JSON.putObject(embed, "thumbnail", thumbObj);
        }

        // Image
        if (imageUrl != null)
        {
            long imgObj = JSON.createObject();
            imgObj = JSON.put(imgObj, "url", imageUrl);
            embed = JSON.putObject(embed, "image", imgObj);
        }

        // Author
        if (authorName != null)
        {
            long authorObj = JSON.createObject();
            authorObj = JSON.put(authorObj, "name", authorName);
            if (authorUrl != null) authorObj = JSON.put(authorObj, "url", authorUrl);
            if (authorIconUrl != null) authorObj = JSON.put(authorObj, "icon_url", authorIconUrl);
            embed = JSON.putObject(embed, "author", authorObj);
        }

        // Footer
        if (footerText != null)
        {
            long footerObj = JSON.createObject();
            footerObj = JSON.put(footerObj, "text", footerText);
            if (footerIconUrl != null) footerObj = JSON.put(footerObj, "icon_url", footerIconUrl);
            embed = JSON.putObject(embed, "footer", footerObj);
        }

        // Fields
        if (fieldsArrayPtr != 0L)
        {
            embed = JSON.putArray(embed, "fields", fieldsArrayPtr);
        }

        // 3. Wrap embed inside array and build main payload
        long finalizedEmbed = JSON.build(embed);
        long embedsArray = JSON.createArray();
        embedsArray = JSON.addArrayElement(embedsArray, finalizedEmbed);
        string.free(finalizedEmbed);

        long payload = JSON.createObject();
        if (content != null) payload = JSON.put(payload, "content", content);
        if (username != null) payload = JSON.put(payload, "username", username);
        if (avatarUrl != null) payload = JSON.put(payload, "avatar_url", avatarUrl);
        payload = JSON.putArray(payload, "embeds", embedsArray);

        long payloadJsonPtr = JSON.build(payload);
        String payloadJson = string.get(payloadJsonPtr);
        string.free(payloadJsonPtr);

        // 4. Build manual HTTP multipart body
        String part1 = "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"payload_json\"\r\n" +
                "Content-Type: application/json\r\n\r\n" +
                payloadJson + "\r\n";

        String part2 = "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"files[0]\"; filename=\"" + attachmentName + "\"\r\n" +
                "Content-Type: " + contentType + "\r\n\r\n";

        byte[] part1Bytes = part1.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] part2Bytes = part2.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] closingBytes = ("\r\n--" + MULTIPART_BOUNDARY + "--\r\n").getBytes(java.nio.charset.StandardCharsets.UTF_8);

        long totalLen = part1Bytes.length + part2Bytes.length + fileBytes.length + closingBytes.length;

        // Allocate off-heap payload buffer
        long multipartBodyPtr = ForeignMemory.allocateNative(totalLen);
        long currentPtr = multipartBodyPtr;

        // Copy segments to off-heap buffer
        for (byte b : part1Bytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : part2Bytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : fileBytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : closingBytes) ForeignMemory.unsafeSet(currentPtr++, b);

        long headersPtr = string.allocate("Content-Type: multipart/form-data; boundary=" + MULTIPART_BOUNDARY);

        try
        {
            transmit(webhookUrl, multipartBodyPtr, headersPtr, totalLen);
        }
        finally
        {
            ForeignMemory.freeNative(multipartBodyPtr);
            string.free(headersPtr);
        }
    }

    @Intention("[String implementation] line 27")
    private static String getString(String attachmentName)
    {
        String contentType = "application/octet-stream";
        if (attachmentName.toLowerCase().endsWith(".png")) contentType = "image/png";
        else if (attachmentName.toLowerCase().endsWith(".jpg") || attachmentName.toLowerCase().endsWith(".jpeg")) contentType = "image/jpeg";
        else if (attachmentName.toLowerCase().endsWith(".pdf")) contentType = "application/pdf";
        else if (attachmentName.toLowerCase().endsWith(".txt") || attachmentName.toLowerCase().endsWith(".log")) contentType = "text/plain";
        return contentType;
    }

    /**
     * Sends a message with a text file attachment to a Discord Webhook.
     * Uses manual off-heap multipart/form-data formatting.
     */
    @Intention("[String implementation] line 27")
    public static void sendTextFile(
        String webhookUrl, String content, String username, String avatarUrl,
        String filename, String fileContent
    ) {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        // 1. Build payload JSON string
        long obj = JSON.createObject();
        if (content != null) obj = JSON.put(obj, "content", content);
        if (username != null) obj = JSON.put(obj, "username", username);
        if (avatarUrl != null) obj = JSON.put(obj, "avatar_url", avatarUrl);
        long payloadJsonPtr = JSON.build(obj);
        String payloadJson = string.get(payloadJsonPtr);
        string.free(payloadJsonPtr);

        // 2. Build manual HTTP multipart body

        // JSON Part

        String sb = "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"payload_json\"\r\n" +
                "Content-Type: application/json\r\n\r\n" +
                payloadJson + "\r\n" +

                // File Part
                "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"files[0]\"; filename=\"" + filename + "\"\r\n" +
                "Content-Type: text/plain\r\n\r\n" +
                fileContent + "\r\n" +

                // End Boundary
                "--" + MULTIPART_BOUNDARY + "--\r\n";

        // 3. Allocate off-heap payload
        long multipartPayloadPtr = string.allocate(sb);
        long headersPtr = string.allocate("Content-Type: multipart/form-data; boundary=" + MULTIPART_BOUNDARY);

        try
        {
            transmit(webhookUrl, multipartPayloadPtr, headersPtr, 0L);
        }
        finally
        {
            string.free(multipartPayloadPtr);
            string.free(headersPtr);
        }
    }

    /**
     * Sends a message with a binary file attachment (like PNG, PDF) to a Discord Webhook.
     * Packages the binary contents safely off-heap with a custom multipart body.
     */
    @Intention("[String implementation] line 27")
    public static void sendFile(
        String webhookUrl, String content, String username, String avatarUrl,
        String filePath
    ) {
        if (webhookUrl == null || webhookUrl.isEmpty())
        {
            throw new APIException("Webhook URL cannot be null or empty.");
        }

        // 1. Resolve file details
        java.io.File file = new java.io.File(filePath);
        if (!file.exists())
        {
            throw new APIException("File does not exist: " + filePath);
        }
        String filename = file.getName();
        byte[] fileBytes;
        try
        {
            fileBytes = java.nio.file.Files.readAllBytes(file.toPath());
        }
        catch (Exception e)
        {
            throw new APIException("Failed to read file: " + filePath, e);
        }

        String contentType = "application/octet-stream";
        if (filename.toLowerCase().endsWith(".png")) contentType = "image/png";
        else if (filename.toLowerCase().endsWith(".jpg") || filename.toLowerCase().endsWith(".jpeg")) contentType = "image/jpeg";
        else if (filename.toLowerCase().endsWith(".pdf")) contentType = "application/pdf";
        else if (filename.toLowerCase().endsWith(".txt") || filename.toLowerCase().endsWith(".log")) contentType = "text/plain";

        // 2. Build payload JSON string
        long obj = JSON.createObject();
        if (content != null) obj = JSON.put(obj, "content", content);
        if (username != null) obj = JSON.put(obj, "username", username);
        if (avatarUrl != null) obj = JSON.put(obj, "avatar_url", avatarUrl);
        long payloadJsonPtr = JSON.build(obj);
        String payloadJson = string.get(payloadJsonPtr);
        string.free(payloadJsonPtr);

        // 3. Formulate multipart components (as strings/bytes)
        String part1 = "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"payload_json\"\r\n" +
                "Content-Type: application/json\r\n\r\n" +
                payloadJson + "\r\n";

        String part2 = "--" + MULTIPART_BOUNDARY + "\r\n" +
                "Content-Disposition: form-data; name=\"files[0]\"; filename=\"" + filename + "\"\r\n" +
                "Content-Type: " + contentType + "\r\n\r\n";

        byte[] part1Bytes = part1.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] part2Bytes = part2.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] closingBytes = ("\r\n--" + MULTIPART_BOUNDARY + "--\r\n").getBytes(java.nio.charset.StandardCharsets.UTF_8);

        long totalLen = part1Bytes.length + part2Bytes.length + fileBytes.length + closingBytes.length;

        // 4. Allocate off-heap payload buffer
        long multipartBodyPtr = ForeignMemory.allocateNative(totalLen);
        long currentPtr = multipartBodyPtr;

        // Copy segments to off-heap buffer
        for (byte b : part1Bytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : part2Bytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : fileBytes) ForeignMemory.unsafeSet(currentPtr++, b);
        for (byte b : closingBytes) ForeignMemory.unsafeSet(currentPtr++, b);

        long headersPtr = string.allocate("Content-Type: multipart/form-data; boundary=" + MULTIPART_BOUNDARY);

        try
        {
            transmit(webhookUrl, multipartBodyPtr, headersPtr, totalLen);
        }
        finally
        {
            ForeignMemory.freeNative(multipartBodyPtr);
            string.free(headersPtr);
        }
    }

    /**
     * Performs direct HTTP POST downcall using FFM libcurl.
     */
    @Intention("[String implementation] line 27")
    private static void transmit(String url, long payloadPtr, long headersPtr, long bodyLen)
    {
        long resPtr = 0L;
        long urlPtr = 0L;
        long methodPtr = 0L;
        try
        {
            urlPtr = string.allocate(url);
            methodPtr = string.allocate("POST");
            if (headersPtr == 0L)
            {
                System.out.println("[Discord Webhook Payload]: " + string.get(payloadPtr));
            }
            resPtr = HTTPClient.request(methodPtr, urlPtr, headersPtr, payloadPtr, bodyLen);
            System.out.println("[Discord Webhook Response]: " + (resPtr != 0L ? string.get(resPtr) : "NULL"));
        }
        catch (Throwable t)
        {
            throw new APIException("Failed to transmit Discord webhook request.", t);
        }
        finally
        {
            if (urlPtr != 0L)
            {
                string.free(urlPtr);
            }
            if (methodPtr != 0L)
            {
                string.free(methodPtr);
            }
            if (resPtr != 0L)
            {
                string.free(resPtr);
            }
        }
    }
}
