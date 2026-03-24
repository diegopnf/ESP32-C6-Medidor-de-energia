module "my_database" {
  source = "./modules/database"
}

module "my_iot" {
  source       = "./modules/iot"
  device_id    = "medidor-esp32c6-01"
  iam_role_arn = aws_iam_role.iot_to_dynamo_role.arn
}