output "dynamodb_table_name" {
  value = module.database.table_name
}

output "iot_thing_name" {
  value = module.iot.thing_name
}

output "s3_bucket_ota" {
  value = aws_s3_bucket.firmware.id
}