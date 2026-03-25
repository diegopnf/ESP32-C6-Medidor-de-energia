resource "aws_s3_bucket" "firmware_ota" {
  bucket = "firmware-ota-${var.project_name}"

  tags = {
    Name    = "Firmware OTA Repository"
    Project = var.project_name
  }
}

# Bloqueio de acesso público obrigatório por segurança
resource "aws_s3_bucket_public_access_block" "firmware_ota_block" {
  bucket = aws_s3_bucket.firmware_ota.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

output "bucket_id" {
  value = aws_s3_bucket.firmware_ota.id
}

output "bucket_arn" {
  value = aws_s3_bucket.firmware_ota.arn
}